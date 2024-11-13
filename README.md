# Digital Twins: estratégias para redução de tempo de resposta utilizando computação em borda 
## TCC 2024 - 2º Semestre - Documentação repositório
## Introdução

Este repositório reúne os itens básicos para replicação dos experimentos realizados no TCC, como o código da entidade digital(Unity), o código da entidade física(Arduino), juntamente com o json do postman para realizar as requisições e configurações do FIWARE, seja na nuvem e na borda
## Desafio identificado (problema)
<img src="./problema.gif" alt="Meu GIF" width="300">

## Objetivo

Este trabalho tem por objetivo dar continuidade a um TCC anterior, de forma a reduzir o tempo de resposta através de estratégias e/ou conceitos de computação em borda e mudança de fluxo

## Hierarquia de pastas

- RoboArduino
  - **New arch**
    - **code new arch**
      - `new_arch`
      - `new_arch_final`
      - `new_arch_test`
  - **Old arch**
    - `roboArmMQTT.cpp`
    - `libraries`

- Robo3d
  - *Assets*
  - *Library*
  - *ProjectSettings*
  - *UserSettings*
  - `Robot_3D.sln`

- `doc_tecnica_postman.json`

## Hardware

Foi utilizado todos os equipamentos da empresa Robocore, chamado RoboARM
Para uma replicação fidedigna é aconselhável a utilização dos mesmos equipamentos

Para mais informações, segue o link com mais especificações do produto [RoboARM](https://www.robocore.net/robotica-robocore/braco-robotico-roboarm?srsltid=AfmBOopRhiBg6qb-HHfhY6gMhe8Fef5xP-YneHdydpPNQUWqTDpRnmcw)

## Especificações

| Software/Biblioteca       | Versão           |
|---------------------------|------------------|
| Eclipse Mosquitto         | 2.0.18           |
| IBM SPSS Statistics       | 30.0.0.0 (Avaliação) |
| IoT Agent                 | 3.5.0            |
| Matplotlib                | 3.9.2            |
| MongoDB                   | 4.4              |
| Orion Context Broker      | 3.11             |
| Oracle VM VirtualBox      | 7.0.20           |
| Pandas                    | 2.2.2            |
| Python                    | 3.12.5           |
| Seaborn                   | 0.13.2           |
| STH-Comet                 | 2.11.0           |
| Unity                     | 2022.3.12f1      |


## Funcionamento

Necessário clonar o repositório e seguir os passos:

1 - Importar a `doc_tecnica_postman.json` no aplicativo do postman, e executar todos os passos, se atentando sempre para o IP configurado*;

2 - Carregar o arduino com o código da nova ou antiga arquitetura;

3 - Configurar o Unity apontando para o IP configurado*

Obs. 1: Lembrando que esse IP configurado é o serviço de nuvem configurado, ou IP da máquina virtual para replicações no ambiente de borda.

Obs. 2: Somente a nova arquitetura tem suporte para o Joystick

## Link para visualização do RoboARM

Link público no youtube: to-do


