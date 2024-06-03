# Código baseado em https://docs.python.org/3.6/library/asyncio-stream.html#tcp-echo-client-using-streams
import asyncio
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
import os
import datetime
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import dh
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.backends import default_backend

from cryptography.hazmat.primitives.asymmetric import padding

from cryptography import x509
import datetime
import cert 

conn_cnt = 0
conn_port = 8443
max_msg_size = 9999

p = 0xFFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF6955817183995497CEA956AE515D2261898FA051015728E5A8AACAA68FFFFFFFFFFFFFFFF
g = 2

pn = dh.DHParameterNumbers(p,g)
Parameters = pn.parameters()

message_queues = {}

class ServerWorker:
    """ Classe que implementa a funcionalidade do SERVIDOR. """
    def __init__(self, cnt, addr=None, key=None):   
        """ Construtor da classe. """
        self.id = cnt
        self.addr = addr
        self.msg_cnt = 0
        self.pk_DH = Parameters.generate_private_key()
        self.pk_RSA = self.load_private_key("projCA/MSG_SERVER.key")
        self.pubKey_ClientDH = None
        self.aesgcm = None

    def parseCert(self, cert):
        subject = {}
        for attr in cert.subject:
            subject[attr.oid._name] = attr.value
        
        return subject

    def load_private_key(self, filename):
        """Carrega a chave privada do arquivo."""
        with open(filename, 'rb') as key_file:
            key_data = key_file.read()
        
        private_key = serialization.load_pem_private_key(
            key_data,
            password=b"1234", 
            backend=default_backend()
        )
        return private_key

    #---------------

    def send_message(self, uid, subject, sender, message, sign):
        # Create message object
        message = {
            'num' : 1,
            'sender': sender,
            'subject': subject,
            'message': message,
            'time': datetime.datetime.now(),
            'sign': sign,
            'read': False
        }

        # Add message to recipient's queue
        if uid in message_queues:
            message['num'] = len(message_queues[uid]) + 1
            message_queues[uid].append(message)
        else:
            message_queues[uid] = [message]

        print("Message sent successfully!")
        
    def get_unread_messages(self, uid):
        if uid in message_queues:
            unread_messages = [msg for msg in message_queues[uid] if not msg['read']]
            if len(unread_messages) == 0:
                return "No unread messages."
            formatted_messages = []
            for msg in unread_messages:
                num = msg['num']
                sender = msg['sender']
                timestamp = msg['time']
                subject = msg['subject']
                formatted_msg = f"{num}:{sender}:{timestamp}:{subject}"
                formatted_messages.append(formatted_msg)
            return "\n".join(formatted_messages)
        else:
            return "User not found."

    def get_message_by_number(self, uid, message_num):
        num = int(message_num)
        if uid in message_queues:
            message_queue = message_queues[uid]
            if isinstance(num, int) and 1 <= num <= len(message_queue):
                message = message_queue[num - 1]
                message['read'] = True  # Mark the message as read
                #return message['message']
                return cert.mkpair(message['message'].encode(), message['sign'])
            else:
                return None
        else:
            return None
        
    #---------------

    def process(self, msg):
        """ Processa uma mensagem (`bytestring`) enviada pelo CLIENTE.
            Retorna a mensagem a transmitir como resposta (`None` para
            finalizar ligação) """
        self.msg_cnt += 1
        
        if self.msg_cnt == 1:
            try:
                # Deserialize the public key from the received message
                pub_key = serialization.load_pem_public_key(
                    msg,
                    backend=default_backend()
                )
            except Exception as e:
                print(f"Error deserializing public key: {e}")
                return None
            
            self.pubKey_ClientDH = pub_key

            pub_key = self.pk_DH.public_key()
            pub_key_serialized = pub_key.public_bytes(
                                 encoding=serialization.Encoding.PEM,
                                 format=serialization.PublicFormat.SubjectPublicKeyInfo
                                 )
            
            pair_pubkeys = cert.mkpair(pub_key_serialized,msg)
            signServer = self.pk_RSA.sign(
                            pair_pubkeys,
                            padding.PSS(
                                mgf=padding.MGF1(hashes.SHA256()),
                                salt_length=padding.PSS.MAX_LENGTH
                            ),
                            hashes.SHA256()
                        )
            

            new_msg = cert.mkpair(cert.mkpair(pub_key_serialized, signServer), cert.serialize_certificate(cert.cert_load("projCA/MSG_SERVER.crt")))
        elif self.msg_cnt == 2:
            signClient, certClient_serialized = cert.unpair(msg)

            certClient = cert.deserialize_certificate(certClient_serialized)

            pub_key = self.pk_DH.public_key()
            pub_key_serialized_Server = pub_key.public_bytes(
                                 encoding=serialization.Encoding.PEM,
                                 format=serialization.PublicFormat.SubjectPublicKeyInfo
                                 )
            pub_key_serialized_Client = self.pubKey_ClientDH.public_bytes(
                                        encoding=serialization.Encoding.PEM,
                                        format=serialization.PublicFormat.SubjectPublicKeyInfo
                                        )
            
            pair_pubkeys = cert.mkpair(pub_key_serialized_Server,pub_key_serialized_Client)
            ca_crt = cert.cert_load("projCA/MSG_CA.crt")

            if(cert.valida_certALICE(ca_crt ,certClient)):
                pubkey_ClientRSA = certClient.public_key()

                pubkey_ClientRSA.verify(signClient,
                        pair_pubkeys,        
                        padding.PSS(
                        mgf=padding.MGF1(hashes.SHA256()),
                        salt_length=padding.PSS.MAX_LENGTH
                        ),
                        hashes.SHA256()
                )

                shared_key = self.pk_DH.exchange(self.pubKey_ClientDH)

                derived_key = HKDF(
                    algorithm=hashes.SHA256(),
                    length=32,
                    salt=None,
                    info=b'handshake data',
                ).derive(shared_key)

                #print("Derived key:", derived_key)

                self.aesgcm = AESGCM(derived_key)

                txt = "Certifacate is valid!"
                new_msg = txt.encode()
                nonce = os.urandom(12)
                ciphertext = self.aesgcm.encrypt(nonce, new_msg, None)
                new_msg = nonce + ciphertext
                
            else: 
                print("ERRO na validação do certificado!")
                new_msg = ""

        else:
            nonce = msg[:12]
            ciphertext = msg[12:]

            try:
                plaintext = self.aesgcm.decrypt(nonce, ciphertext, None)

                new_msg, clientStuff = cert.unpair(plaintext)

                txt = new_msg.decode().strip()
                msgA = txt.split()

                help = "Available commands:\n- send <UID> <SUBJECT> (Sends a message with a SUBJECT to the user UID)\n- askqueue (Requests the list of unread messages from the user's queue)\n- getmsg <NUM> (Requests a message, with the number NUM)\n- help (Shows program instructions)"

                certClient_deserialized = cert.deserialize_certificate(clientStuff)
                certName = self.parseCert(certClient_deserialized)["pseudonym"]

                with open("log.txt", "a") as file:
                    file.write(f'[{self.msg_cnt}] {certName}: {txt}\n')

                if txt.startswith("send"):
                    if len(new_msg.split()) < 3:
                        response = "MSG RELAY SERVICE: verification error!"
                    else:
                        signClient, certClient_serialized = cert.unpair(clientStuff)
                        certClient_deserialized = cert.deserialize_certificate(certClient_serialized)
                        ca_crt = cert.cert_load("projCA/MSG_CA.crt") 

                        uid = msgA[1]
                        subject = msgA[2]
                        message = " ".join(msgA[3:])

                        if(cert.valida_certALICE(ca_crt, certClient_deserialized)):
                            pubkey_ClientRSA = certClient_deserialized.public_key()

                            pubkey_ClientRSA.verify(
                                    signClient,
                                    cert.mkpair(message.encode(), uid.encode()),        
                                    padding.PSS(
                                    mgf=padding.MGF1(hashes.SHA256()),
                                    salt_length=padding.PSS.MAX_LENGTH
                                    ),
                                    hashes.SHA256()
                            )

                            print(f"Received <send> command from client: send {uid} {subject} {message}")

                            self.send_message(uid, subject, self.parseCert(certClient_deserialized)["pseudonym"], message, clientStuff)
                            response = "Message received successfully."

                elif txt.startswith("askqueue"):
                    if len(msgA) != 1:
                        response = "MSG RELAY SERVICE: verification error!"
                    else:
                        certClient_deserialized = cert.deserialize_certificate(clientStuff)
                        userid = self.parseCert(certClient_deserialized)["pseudonym"]
                        response = "Messages: " + self.get_unread_messages(userid)
                    
                elif txt.startswith("getmsg"):
                    if len(msgA) != 2:
                        response = "MSG RELAY SERVICE: verification error!"
                    else:
                        num = txt.split()[1]
                        certClient_deserialized = cert.deserialize_certificate(clientStuff)
                        result = self.get_message_by_number(self.parseCert(certClient_deserialized)["pseudonym"], num)
                        if result is None:
                            response = "MSG RELAY SERVICE: unknown message!"
                        else:
                            #response = "Message " + num + ": " + result
                            response = result
                    
                elif txt.startswith("help"):
                    if len(msgA) != 1:
                        response = "MSG RELAY SERVICE: verification error!"
                    else:
                        response = help
                else:
                    response = "MSG RELAY SERVICE: command error!" + "#" + help
            except Exception as e:
                print(f"Error decrypting message: {e}")
                return None

            if isinstance(response, str):
                print('%d: %r' % (self.id, response))

                with open("log.txt", "a") as file:
                    file.write(f'[{self.msg_cnt}] Server: {response}\n')

                new_msg = response.encode()
            else:
                new_msg = response
    
            # Encrypt the response before sending it back to the client
            nonce = os.urandom(12)
            ciphertext = self.aesgcm.encrypt(nonce, new_msg, None)
            new_msg = nonce + ciphertext
            pass
        
        
        return new_msg if len(new_msg) > 0 else None
        


#
#
# Funcionalidade Cliente/Servidor
#
# obs: não deverá ser necessário alterar o que se segue
#
async def handle_echo(reader, writer):
    global conn_cnt
    conn_cnt +=1
    addr = writer.get_extra_info('peername')
    srvwrk = ServerWorker(conn_cnt, addr)
    data = await reader.read(max_msg_size)
    while True:
        if not data: continue
        if data[:1]==b'\n': break
        data = srvwrk.process(data)
        if not data: break
        writer.write(data)
        await writer.drain()
        data = await reader.read(max_msg_size)
    print("[%d]" % srvwrk.id)
    writer.close()


def run_server():
    loop = asyncio.new_event_loop()
    coro = asyncio.start_server(handle_echo, '127.0.0.1', conn_port)
    server = loop.run_until_complete(coro)
    # Serve requests until Ctrl+C is pressed
    print('Serving on {}'.format(server.sockets[0].getsockname()))
    print('  (type ^C to finish)\n')
    try:
        loop.run_forever()
    except KeyboardInterrupt:
        pass
    # Close the server
    server.close()
    loop.run_until_complete(server.wait_closed())
    loop.close()
    print('\nFINISHED!')

run_server()