# Marco-3
# Projeto Coprocessador - Sistemas Digitais (PBL)

Aplicação final do sistema de classificação de dígitos com interface VGA,
integrando o co-processador ELM desenvolvido nos marcos anteriores.

---

## Sumário

- [1. Introdução](#1-introdução)
  - [1.1. Sobre o Projeto](#11-sobre-o-projeto)
  - [1.2. Contexto do Problema](#12-contexto-do-problema)
  - [1.3. Requisitos Principais](#13-requisitos-principais)
- [2. Fundamentação Teórica](#2-fundamentação-teórica)
  - [2.1. IP-Core VGA](#21-ip-core-vga)
  - [2.2. Métricas de Benchmark](#22-métricas-de-benchmark)
  - [2.3. Protocolo do Mouse Linux](#23-protocolo-do-mouse-linux)
- [3. Materiais e Métodos](#3-materiais-e-métodos)
  - [3.1. DE1-SoC](#31-de1-soc)
  - [3.2. IP-Core VGA](#32-ip-core-vga)
  - [3.3. Mouse](#33-mouse)
  - [3.4. Driver Assembly — Marco 2](#34-driver-assembly--marco-2)
  - [3.5. Metodologia](#35-metodologia)
- [4. Arquitetura da Solução](#4-arquitetura-da-solução)
  - [4.1. Visão Geral](#41-visão-geral)
  - [4.2. Núcleo Comum de Inferência](#42-núcleo-comum-de-inferência)
- [5. Descrição da Solução](#5-descrição-da-solução)
  - [5.1. Modo Arquivo](#51-modo-arquivo)
  - [5.2. Modo Desenho](#52-modo-desenho)
  - [5.3. Modo Benchmark](#53-modo-benchmark)
- [6. Modo de Uso](#6-modo-de-uso)
- [7. Testes](#7-testes)
  - [7.1. Ambiente e Configuração de Teste](#71-ambiente-e-configuração-de-teste)
  - [7.2. Casos de Teste / Cenários](#72-casos-de-teste--cenários)
  - [7.3. Procedimento](#73-procedimento)
- [8. Resultados](#8-resultados)
  - [8.1. Resultados — Modo Arquivo](#81-resultados--modo-arquivo)
  - [8.2. Resultados — Modo Desenho](#82-resultados--modo-desenho)
  - [8.3. Resultados — Modo Benchmark](#83-resultados--modo-benchmark)
  - [8.4. Análise / Discussão dos Resultados](#84-análise--discussão-dos-resultados)
- [9. Referências](#9-referências)

---

## 1. Introdução

Este documento descreve o desenvolvimento do Marco 3 de um sistema embarcado para classificação de dígitos numéricos. O sistema completo combina um co-processador implementado em Verilog na FPGA Cyclone V da placa DE1-SoC, desenvolvido no Marco 1, com um driver em linguagem Assembly ARMv7 — responsável pela comunicação entre o HPS e o hardware via MMIO, desenvolvido e validado no Marco 2 —, integrado a uma aplicação completa em C.
O objetivo do Marco 3 é entregar essa aplicação final, voltada diretamente ao usuário, incorporando uma interface visual via VGA e três modos distintos de utilização: classificação a partir de arquivo, classificação a partir de desenho feito com o mouse, e um modo de benchmark automatizado para avaliação de desempenho do sistema.

### 1.1. Sobre o Projeto

O sistema classifica dígitos manuscritos (0–9) usando o modelo ELM
executado diretamente na FPGA. A camada desenvolvida neste marco é a
aplicação voltada ao usuário: recebe as entradas, prepara os dados e exibe
o resultado da inferência. O driver entregue no Marco 2 continua sendo
utilizado sem modificações em sua lógica de comunicação com o hardware.

A principal novidade é a integração com o IP-Core VGA fornecido para a
disciplina, que permite tanto exibir imagens no monitor quanto capturar,
via mouse, o desenho produzido pelo usuário para posterior envio ao
co-processador.

### 1.2. Contexto do problema

No Marco 2, o driver já operava corretamente, com acurácia acima de 80% nos
testes realizados, mas a inferência era acionada apenas por um programa de
teste simples em C — sem interface visual e sem flexibilidade para o
usuário escolher a imagem a ser classificada. Concluir o projeto de forma
completa exigia ir além dessa validação mínima.

O Marco 3 ataca exatamente essa lacuna. Os principais desafios enfrentados
foram:

- Integrar o IP-Core VGA ao projeto já existente sem comprometer o
  funcionamento do que já estava validado;
- Garantir que a imagem exibida no monitor correspondesse exatamente à
  imagem enviada ao co-processador;
- Converter o desenho produzido pelo usuário via mouse para o formato de
  entrada esperado pela rede neural;
- Medir o tempo de execução de forma precisa no modo benchmark, de modo que
  as métricas refletissem o comportamento real do sistema.

### 1.3. Requisitos Principais

- **Integração do IP-Core VGA** — conectar o módulo de vídeo fornecido pelo
  tutor ao projeto existente, sem necessidade de alterar o co-processador.
- **Modo Arquivo** — ler uma imagem em formato `.bin` ou `.png` armazenada e
  classificá-la utilizando o co-processador.
- **Modo Desenho** — permitir que o usuário desenhe um dígito na tela
  usando o mouse, e classificar o resultado.
- **Modo Benchmark** — executar a classificação de múltiplas imagens
  automaticamente, medindo o desempenho do sistema.
- **Interface via linha de comando** — permitir a escolha do modo de
  operação e a passagem de parâmetros no terminal, sem alterar o código.
- **Exibição via VGA** — mostrar a imagem a ser classificada no monitor
  antes da inferência, fornecendo feedback visual ao usuário.
- **Comunicação via driver do Marco 2** — utilizar as funções Assembly já
  desenvolvidas para comunicação com o co-processador.
- **Cálculo de métricas** — medir desempenho através de acurácia, latência
  média, desvio padrão e throughput.
- **Log de resultados em CSV** — salvar os resultados da execução em
  arquivo CSV para possibilitar análises posteriores.

---

## 2. Fundamentação Teórica

### 2.1. IP-Core VGA

O IP-Core VGA permite exibir imagens e informações diretamente no monitor
conectado à DE1-SoC, gerando os sinais de vídeo necessários para a saída
VGA. Esse módulo foi disponibilizado para a disciplina.

A comunicação entre o HPS e o IP-Core ocorre via MMIO, por meio de
registradores mapeados em memória, permitindo o envio individual de pixels
para a tela. Cada pixel é representado por uma palavra de 32 bits: os bits
mais significativos codificam as coordenadas X e Y, enquanto os bits menos
significativos definem a cor em RGB (3 bits para vermelho, 3 para verde e
2 para azul).

O módulo foi utilizado principalmente para exibir imagens do conjunto
MNIST e para fornecer feedback visual durante o modo de desenho.

### 2.2. Métricas de Benchmark

As métricas adotadas para avaliação de desempenho são as usualmente
empregadas em aplicações de classificação de imagens:

- **Acurácia** — percentual de imagens classificadas corretamente em
  relação ao total processado.
- **Latência** — tempo necessário para processar uma única imagem,
  medido durante a etapa de inferência.
- **Desvio padrão da latência** — utilizado para avaliar a estabilidade do
  tempo de execução entre inferências distintas.
- **Throughput** — quantidade de imagens classificadas por segundo,
  indicando a capacidade geral de processamento da solução.

### 2.3. Protocolo do Mouse Linux

No Linux, dispositivos de hardware são representados como arquivos,
seguindo o princípio Unix de que "tudo é arquivo". Um mouse USB conectado
tem sua interface disponibilizada pelo kernel através do arquivo
`/dev/input/mice`, permitindo que aplicações leiam seus eventos com
operações convencionais de entrada e saída.

O protocolo envia pacotes de 3 bytes a cada movimento ou clique: o
primeiro byte contém o estado dos botões e flags de controle; os dois
bytes seguintes representam os deslocamentos relativos nos eixos X e Y,
variando de -128 a 127. Esses valores indicam apenas o quanto o mouse se
moveu desde a última leitura — não uma posição absoluta na tela.

Para obter a posição atual do cursor, o software acumula os deslocamentos
recebidos a cada leitura. No eixo Y, o sinal do movimento precisa ser
invertido, já que o protocolo do mouse trata valores positivos como
"para cima", enquanto o sistema de coordenadas do VGA trata valores
positivos como "para baixo". Adicionalmente, a posição calculada deve ser
limitada à área de desenho, evitando que o cursor saia do canvas.

---

## 3. Materiais e Métodos

### 3.1. DE1-SoC

A DE1-SoC permanece como a plataforma principal do projeto, combinando um
processador ARM Cortex-A9 dual-core (executando Linux embarcado) com uma
FPGA Cyclone V responsável pelos módulos implementados em hardware.

Em relação ao Marco 2, foram adicionados três novos PIOs para comunicação
com o controlador VGA, mapeados nos offsets `0x30`, `0x40` e `0x50` da
Lightweight Bridge. Um monitor VGA externo e um mouse USB foram conectados
para interação com o sistema, e um novo arquivo `.sof` foi gerado e
gravado na FPGA para refletir essas alterações.

### 3.2. IP-Core VGA

O IP-Core VGA é responsável pela geração da saída de vídeo da DE1-SoC,
produzindo automaticamente os sinais de sincronismo horizontal (hsync) e
vertical (vsync), além dos sinais de cor enviados ao monitor.

O módulo utiliza uma memória dual-port de 320×240 posições, com 9 bits de
cor por pixel. Durante a exibição, cada pixel é ampliado para uma área
2×2, resultando em uma resolução final de 640×480 no monitor VGA.

A comunicação com o HPS ocorre via registradores mapeados em memória, para
onde são enviadas as coordenadas e a cor de cada pixel. Os valores de
posição X, posição Y e componentes RGB são combinados em uma única palavra
de 32 bits antes da escrita no registrador de dados.

### 3.3. Mouse

O mouse é o principal dispositivo de entrada no modo de desenho. Ao ser
conectado à DE1-SoC, é reconhecido automaticamente pelo Linux embarcado,
que disponibiliza seus eventos via `/dev/input/mice`, sem necessidade de
configuração adicional.

O movimento relativo reportado pelo mouse é acumulado para atualizar a
posição do cursor em um canvas de 224×224 pixels exibido no VGA. Esse
canvas representa uma grade de 28×28 células — compatível com o formato de
entrada do conjunto MNIST — onde cada célula ocupa uma região de 8×8
pixels na tela.

Enquanto o botão esquerdo permanece pressionado, a célula correspondente à
posição atual do cursor é preenchida (tanto na estrutura interna do canvas
quanto na imagem exibida no VGA). O botão direito finaliza o desenho: ao
ser acionado, o programa encerra a captura de eventos do mouse, extrai a
matriz 28×28 resultante, aplica o filtro de suavização (blur) e envia os
dados ao co-processador para inferência.

### 3.4. Driver Assembly — Marco 2

O driver Assembly desenvolvido no Marco 2 foi reaproveitado integralmente
no Marco 3. As funções de mapeamento de memória (`mapeia_memoria()`),
reset do co-processador (`reset_coprocessador()`) e início da inferência
(`comeca_infer()`) permaneceram inalteradas.

A principal mudança ocorreu nas funções de transferência de dados, que
passaram a receber buffers já carregados em memória em vez de caminhos de
arquivo — por exemplo, `store_bias(base, buf_bias)`. A função
`store_pesos` também passou a receber um terceiro parâmetro indicando a
quantidade de valores a enviar, como em
`store_pesos(base, buf_pesos, 100352)`. Essa mudança permitiu separar o
tempo de pré-processamento e leitura de arquivos do tempo efetivo de
comunicação com o hardware, tornando as medições de desempenho mais
precisas.

### 3.5. Metodologia

*(seção a ser escrita)*

---

## 4. Arquitetura da Solução

### 4.1. Visão Geral

*(seção a ser escrita)*

### 4.2. Núcleo Comum de Inferência

*(seção a ser escrita)*

---

## 5. Descrição da Solução

### 5.1. Modo Arquivo

*(seção a ser escrita)*

### 5.2. Modo Desenho

*(seção a ser escrita)*

### 5.3. Modo Benchmark

*(seção a ser escrita)*

---

## 6. Modo de Uso

*(seção a ser escrita)*

---

## 7. Testes

### 7.1. Ambiente e Configuração de Teste

*(seção a ser escrita)*

### 7.2. Casos de Teste / Cenários

*(seção a ser escrita)*

### 7.3. Procedimento

*(seção a ser escrita)*

---

## 8. Resultados

### 8.1. Resultados — Modo Arquivo

*(seção a ser escrita)*

### 8.2. Resultados — Modo Desenho

*(seção a ser escrita)*

### 8.3. Resultados — Modo Benchmark

*(seção a ser escrita)*

### 8.4. Análise / Discussão dos Resultados

*(seção a ser escrita)*

---

## 9. Referências

*(seção a ser escrita)*
