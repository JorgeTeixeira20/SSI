from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.backends import default_backend
from cryptography import x509
import datetime

# Validation

def cert_load(fname):
    """lê certificado de ficheiro"""
    with open(fname, "rb") as fcert:
        cert = x509.load_pem_x509_certificate(fcert.read())
    return cert


def cert_validtime(cert, now=None):
    """valida que 'now' se encontra no período
    de validade do certificado."""
    if now is None:
        now = datetime.datetime.now(tz=datetime.timezone.utc)
    if now < cert.not_valid_before_utc or now > cert.not_valid_after_utc:
        raise x509.verification.VerificationError(
            "Certificate is not valid at this time"
        )


def cert_validsubject(cert, attrs=[]):
    """verifica atributos do campo 'subject'. 'attrs'
    é uma lista de pares '(attr,value)' que condiciona
    os valores de 'attr' a 'value'."""
    print(cert.subject)
    for attr in attrs:
        if cert.subject.get_attributes_for_oid(attr[0])[0].value != attr[1]:
            raise x509.verification.VerificationError(
                "Certificate subject does not match expected value"
            )


def cert_validexts(cert, policy=[]):
    """valida extensões do certificado.
    'policy' é uma lista de pares '(ext,pred)' onde 'ext' é o OID de uma extensão e 'pred'
    o predicado responsável por verificar o conteúdo dessa extensão."""
    for check in policy:
        ext = cert.extensions.get_extension_for_oid(check[0]).value
        if not check[1](ext):
            raise x509.verification.VerificationError(
                "Certificate extensions does not match expected value"
            )


def valida_certALICE(ca_cert, cert):
    try:
        #cert = cert_load("Alice/MSG_CLI1.crt")
        # obs: pressupõe que a cadeia de certifica só contém 2 níveis
        cert.verify_directly_issued_by(ca_cert)
        # verificar período de validade...
        cert_validtime(cert)
        #print("Certificate is valid!")
        return True
    except:
        #print("Certificate is invalid!")
        return False

def valida_certBOB(ca_cert, cert):
    try:
        cert.verify_directly_issued_by(ca_cert)

        # verificar período de validade...
        cert_validtime(cert)

        #print("Certificate is valid!")
        return True
    except:
        #print("Certificate is invalid!")
        return False

# pairs

def mkpair(x, y):
    """produz uma byte-string contendo o tuplo '(x,y)' ('x' e 'y' são byte-strings)"""
    len_x = len(x)
    len_x_bytes = len_x.to_bytes(2, "little")
    return len_x_bytes + x + y

def unpair(xy):
    """extrai componentes de um par codificado com 'mkpair'"""
    len_x = int.from_bytes(xy[:2], "little")
    x = xy[2 : len_x + 2]
    y = xy[len_x + 2 :]
    return x, y

# serialize 

def serialize_certificate(cert):
    """Serialize a certificate into bytes."""
    return cert.public_bytes(encoding=serialization.Encoding.PEM)

def deserialize_certificate(cert_bytes):
    """Deserialize bytes into a certificate."""
    return x509.load_pem_x509_certificate(cert_bytes, default_backend())