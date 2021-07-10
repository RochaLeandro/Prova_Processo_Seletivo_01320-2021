# Processo Seletivo - 01320/2021
> Prova prática do Processo Seletivo - 01320/2021 - Pesquisador I - Firmware (SENAI Florianópolis) - Alisson Lopes Furlani

## Índice
- [Índice](#índice)
- [Descrição](#descrição)
- [Compilação](#compilação)
- [Execução](#execução)
- [Funcionalidades](#funcionalidades)
- [Pendências (TODO)](#pendências-todo)
- [Referências](#referências)
- [Contato](#contato)

## Descrição

tarefa_1, simule a leitura de uma senóide com frequência de 60Hz por um ADC, a amostragem deste sinal deve ser realizada a cada 1ms, e salva em um buffer. O buffer de armazenamento deve possuir 1000 posições. 

tarefa_2, a cada 100ms processe o sinal salvo pela tarefa_1, multiplique o valor amostrado por 3.141592 e guarde esse valor em outro buffer de 1000 posições. Uma amostra nunca deve ser multiplicada por 3.141592 mais de uma vez.

tarefa_3, disponibilize uma interface serial para o usuário, quando ele digitar "obter" recebe os dados processados, e quando digitar "zerar" ele limpa os buffers.

tarefa_4, a cada 3 segundos utiliza a função "vTaskGetRunTimeStats" para apresentar o consumo dos processos, pela a serial.

Prepare uma apresentação mostrando a interação entre as tarefas e explicando o projeto.

## Compilação
Execute _make_ a partir do diretório base

## Execução
Execute _./build/app_ a partir do diretório base (ou _./app_ a partir do direrório _build_)

## Referências
Baseado no exemplo _Posix\_GCC_ do FreeRTOS.

## Contato
Criado por [@Alisson Lopes Furlani](mailto:alisson.furlani@gmail.com).

