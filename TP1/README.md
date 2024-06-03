# Projeto de criptografia aplicada (TP1) - Relatório

## Introdução

Neste relatório apresentamos o desenvolvimento de um serviço de __Message Relay__ (em Python) que permite aos membros do mesmo a troca de mensagens com uma garantia de autenticidade e impossibilidade de intercessão de mensagens

Este serviço é suportado por um servidor responsável por manter o estado da aplicação e interagir com os utilizadores do sistema.

## Objetivo

Este projeto tem como objetivo a implementação de um sistema de comunicação seguro entre membros de uma organização, utilizando a criptografia para assegurar a autenticidade e integridade de todas as mensagens trocadas entre utilizadores. O projeto prevê ainda que seja impossível que utilizadores fora da rede consigam quer intercetar as mensagens, quer sequer conectar-se com o servidor.

## Arquitetura do Sistema

Sistema composto por dois programas: 

- __msg_server.py__  : Servidor responsável por responder às solicitações dos utilizadores e conservar o estado da aplicação.

- __msg_client.py__ : Cliente (executado por cada utilizador) que permite o acesso à funcionalidade oferecida pelo serviço em si.

Além destes programas, a arquitetura do sistema inclui o ficheiro __cert.py__, que desempenha um papel crucial na validação e manipulação de certificados X509. Este fornece funções essenciais para carregar, validar, e serializar certificados, garantindo a autenticidade dos utilizadores e a integridade das comunicações. A abordagem baseada em certificados X509 proporciona uma camada adicional de segurança ao sistema, permitindo uma troca segura de mensagens entre os clientes e o servidor. 


## Funcionalidades Implementadas

1. __Comunicação Protegida__ 

    Todas as comunicações entre clientes e servidor são protegidas contra acesso ilegítimo e manipulação. Sendo feita a verificação dos certificados de ambos os intervenientes na rede aquando da sua ligação, e posteriormente a conexão de ambos com troca de chaves através do algoritmo Diffie–Hellman.

2. __Autenticidade das Mensagens__

    Os clientes têm a possibilidade de verificar a autenticidade das mensagens recebidas de modo a garantir que foram enviadas pelo utilizador correto.

3. __Timestamps Confidenciais__

    O servidor atribui **timestamps** às mensagens recebidas.

4. __Identificação dos Utilizadores__

    A identificação dos utilizadores é feita através de certificados X509, de onde extraimos o atributo **PSEUDONYM** que usamos como UID.

## Implementação

Quanto à implementação dos conceitos em si, optamos por não alterar a parte que efetua a troca de mensagens em si, alterando assim apenas a lógica de como essas mensagens são tratadas quando transitam do cliente para o servidor e vice-versa.
O primeiro passo, depois de feita a conexão, consiste não só na validação dos certificados, mas também na troca e verificação das chaves segundo o algoritmo Diffie–Hellman. Caso exista algum erro neste passo anterior, a conexão é interrompida e o cliente fica então impossibilitado de comunicar com o servidor. Caso a verificação dos certificados e a troca de chaves sejam concluídos com sucesso, passamos então ao parsing das mensagens onde implementamos os comandos pedidos no enunciado, tendo em atenção o facto de todas as mensagens serem encriptadas com as chaves acordadas no passo inicial. Uma __feature__ que achamos importante  salientar é o facto de ao ser utilizado o comando __getmsg__, o cliente verifica se a mensagem que o servidor transmitiu foi efetivamente enviada por quem o servidor comunicou, através de uma validação da assinatura da origem garantindo a autenticidade da mensagem.

Para além das funcionalidades básicas pretendidas, implementamos também um sistema de log que permite ao servidor registar todas as mensagens trocadas entre cliente e servidor. Os detalhes guardados para cada uma destas são o identificador único de mensagem (para facilitar a referência de cada interação), UID do cliente que a enviou e o conteúdo da mesma.

## Melhorias Futuras

A arquitetura do sistema facilita a implementação de melhorias futuras. Algumas delas são:

- Suporte à Certificação: Implementar a funcionalidade do próprio sistema gerar certificados X509.

- Estruturação de mensagens: Utilizar formatos como JSON para estruturar as mensagens do protocolo de comunicação.

- Garantias de Confidencialidade: Implementar técnicas para garantir a confidencialidade das mensagens perante um "servidor curioso".

## Conclusão

Através da implementação deste sistema, pudemos, enquanto grupo, explorar e adquirir conhecimento sobre a importância da existência da criptografia na segurança e autenticidade das comunicações.

Em suma, o projeto atingiu os objetivos básicos esperados existindo a possibilidade de, no futuro, implementar melhorias/desenvolvimentos na parte da criptografia fortalecendo a segurança e a confiabilidade de comunicações no sistema.
