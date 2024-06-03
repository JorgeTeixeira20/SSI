# Código baseado em https://docs.python.org/3.6/library/asyncio-stream.html#tcp-echo-client-using-streams
import asyncio
import os
import sys
import socket
import cryptography
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import dh
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.backends import default_backend

from cryptography.hazmat.primitives.asymmetric import padding
import cert
from cryptography.hazmat.primitives.serialization import pkcs12

conn_port = 8443
max_msg_size = 9999

p = 0xFFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF6955817183995497CEA956AE515D2261898FA051015728E5A8AACAA68FFFFFFFFFFFFFFFF
g = 2

pn = dh.DHParameterNumbers(p,g)
Parameters = pn.parameters()


class Client:
    """ Classe que implementa a funcionalidade de um CLIENTE. """
    def __init__(self, path, sckt=None):
        """ Construtor da classe. """
        self.sckt = sckt
        self.pathtofile = path
        self.msg_cnt = 0
        self.pk_DH = Parameters.generate_private_key()
        self.pk_RSA, self.cert_RSA, self.cert_CA = self.get_userdata(path)
        self.uid = self.parseCert(self.cert_RSA)["pseudonym"]
        self.pubkey_Server = None
        self.aesgcm = None

    def parseCert(self, cert):
        subject = {}
        for attr in cert.subject:
            subject[attr.oid._name] = attr.value
        
        return subject
    
    def get_userdata(self, p12_fname):
        with open(p12_fname, "rb") as f:
            p12 = f.read()
        password = None # p12 não está protegido...
        (private_key, user_cert, [ca_cert]) = pkcs12.load_key_and_certificates(p12, password)
        return (private_key, user_cert, ca_cert)
    
    def distinguish_encode_message(self, message):
        try:
            decoded_str = message.decode()
            return decoded_str
        except UnicodeDecodeError:
            return None

    def process(self, msg=b""):
        self.msg_cnt +=1
        #
        # ALTERAR AQUI COMPORTAMENTO DO CLIENTE
        #
        if self.msg_cnt == 1:
            pub_key = self.pk_DH.public_key()
            new_msg = pub_key.public_bytes(
                        encoding=serialization.Encoding.PEM,
                        format=serialization.PublicFormat.SubjectPublicKeyInfo
                        )

        elif self.msg_cnt == 2:

            pair_PubKey_SignServer, certServer = cert.unpair(msg)
            pubkey_Server_serialized, signServer = cert.unpair(pair_PubKey_SignServer)
            deserialized_cert_server = cert.deserialize_certificate(certServer)

            if (cert.valida_certBOB(self.cert_CA, deserialized_cert_server)):
                try:
                    # Deserialize the public key from the received message
                    pubkey_ServerDH = serialization.load_pem_public_key(
                        pubkey_Server_serialized,
                        backend=default_backend()
                    )
                except Exception as e:
                    print(f"Error deserializing public key: {e}")
                    return None
                
                self.pubkey_Server = pubkey_ServerDH
                pubkey_ServerRSA = deserialized_cert_server.public_key()
                pub_key = self.pk_DH.public_key()
                pubKey_Client_serialized = pub_key.public_bytes(
                                                encoding=serialization.Encoding.PEM,
                                                format=serialization.PublicFormat.SubjectPublicKeyInfo
                                            )

                pair_pubkeys = cert.mkpair(pubkey_Server_serialized,pubKey_Client_serialized)
                pubkey_ServerRSA.verify(
                    signature=signServer,
                    data= pair_pubkeys,
                    padding=padding.PSS(
                        mgf=padding.MGF1(hashes.SHA256()),
                        salt_length=padding.PSS.MAX_LENGTH
                    ),
                    algorithm=hashes.SHA256()
                )

                signClient = self.pk_RSA.sign(
                            pair_pubkeys,
                            padding.PSS(
                                mgf=padding.MGF1(hashes.SHA256()),
                                salt_length=padding.PSS.MAX_LENGTH
                            ),
                            hashes.SHA256()
                        )
                
                new_msg = cert.mkpair(signClient,cert.serialize_certificate(self.cert_RSA))
                shared_key = self.pk_DH.exchange(pubkey_ServerDH)

                # Perform key derivation.
                derived_key = HKDF(
                    algorithm=hashes.SHA256(),
                    length=32,
                    salt=None,
                    info=b'handshake data',
                ).derive(shared_key)

                self.aesgcm = AESGCM(derived_key)

            else:
                print("ERRO na validação do certificado!")
                new_msg = ""

        else:
            nonce = msg[:12]
            ciphertext = msg[12:] 

            plaintext = self.aesgcm.decrypt(nonce, ciphertext, None)
            #msg = plaintext.decode()

            msg = self.distinguish_encode_message(plaintext)
            if msg is None:
                txt, clientStuff = cert.unpair(plaintext)
                signClient, certClient_serialized = cert.unpair(clientStuff)
                certClient_deserialized = cert.deserialize_certificate(certClient_serialized)

                if(cert.valida_certALICE(self.cert_CA, certClient_deserialized)):
                    pubkey_ClientRSA = certClient_deserialized.public_key()

                    try:
                        pubkey_ClientRSA.verify(
                                signClient,
                                cert.mkpair(txt, self.uid.encode()),        
                                padding.PSS(
                                mgf=padding.MGF1(hashes.SHA256()),
                                salt_length=padding.PSS.MAX_LENGTH
                                ),
                                hashes.SHA256()
                        )

                        msg = "Message: " + txt.decode()
                    except cryptography.exceptions.InvalidSignature:
                        msg = "Invalid message."


            if msg.startswith("MSG RELAY SERVICE: command error"):
                msgA = msg.split('#')
                print(msgA[0], file=sys.stderr)
                print(msgA[1])
            elif msg.startswith("MSG RELAY SERVICE"):
                print(msg, file=sys.stderr)
            else:
                print('(%d) %s' % (self.msg_cnt , msg))
 
            print('\nInput message to send (empty to finish)')
            new_msg = input()
            if new_msg == "": return None

            if new_msg.startswith("send") and len(new_msg.split()) == 3:
                print("Write your message (max 1000 bytes, Ctrl + D to end):")
                new_txt = sys.stdin.read(1000)
                print("\nSending ...")

                dest = new_msg.split()[1]

                new_msg = new_msg + " " + new_txt
                
                signClient = self.pk_RSA.sign(
                            cert.mkpair(new_txt.encode(), dest.encode()),
                            padding.PSS(
                                mgf=padding.MGF1(hashes.SHA256()),
                                salt_length=padding.PSS.MAX_LENGTH
                            ),
                            hashes.SHA256()
                        )
                
                new_msg = cert.mkpair(new_msg.encode(), cert.mkpair(signClient,cert.serialize_certificate(self.cert_RSA)))
            else:
                new_msg = cert.mkpair(new_msg.encode(), cert.serialize_certificate(self.cert_RSA))

            if new_msg:
                nonce = os.urandom(12)
                ciphertext = self.aesgcm.encrypt(nonce, new_msg, None)
                new_msg = nonce + ciphertext
        

        return new_msg


#
#
# Funcionalidade Cliente/Servidor
#
# obs: não deverá ser necessário alterar o que se segue
#

async def tcp_echo_client():
    if len(sys.argv) >= 3 and sys.argv[1] == "-user":
        pathtofile = sys.argv[2]
    else:
        pathtofile = "projCA/userdata.p12"

    if not os.path.exists(pathtofile):
        print("User file specified does not exists.")
        return    

    reader, writer = await asyncio.open_connection('127.0.0.1', conn_port)
    addr = writer.get_extra_info('peername')

    client = Client(pathtofile, addr)

    msg = client.process()
    while msg:
        #print("Sending message:", msg)
        writer.write(msg)
        await writer.drain()
        msg = await reader.read(max_msg_size)
        #print("Received message:", msg)
        if msg :
            msg = client.process(msg)
        else:
            break

    writer.write(b'\n')
    print('Socket closed!')
    writer.close()

def run_client():
    loop = asyncio.get_event_loop()
    loop.run_until_complete(tcp_echo_client())


run_client()