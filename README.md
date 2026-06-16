# Marco-3
# Projeto Coprocessador - Sistemas Digitais (PBL)
Aplicação final do sistema de classificação de dígitos com interface VGA,
integrando o co-processador ELM desenvolvido nos marcos anteriores.

---

## Sumário

- [1. Introdução](#1-introdução)
- [2. Sobre o Projeto](#2-sobre-o-projeto)
- [3. Declaração do Problema](#3-declaração-do-problema)
  - [3.1 Contexto](#31-contexto)
  - [3.2 Requisitos Principais](#32-requisitos-principais)
- [4. Fundamentação Teórica](#4-fundamentação-teórica)
  - [4.1 IP-Core VGA](#41-ip-core-vga)
  - [4.2 Modos de Operação](#42-modos-de-operação)
  - [4.3 Métricas de Benchmark](#43-métricas-de-benchmark)
- [5. Materiais e Métodos](#5-materiais-e-métodos)
  - [5.1 DE1-SoC](#51-de1-soc)
  - [5.2 IP-Core VGA](#52-ip-core-vga)
  - [5.3 Driver Assembly — Marco 2](#53-driver-assembly--marco-2)
  - [5.4 Metodologia](#54-metodologia)
- [6. Arquitetura da Solução](#6-arquitetura-da-solução)
  - [6.1 Visão Geral](#61-visão-geral)
  - [6.2 Fluxo de Execução](#62-fluxo-de-execução)
- [7. Descrição da Solução](#7-descrição-da-solução)
  - [7.1 Modo Arquivo](#71-modo-arquivo)
  - [7.2 Modo Desenho](#72-modo-desenho)
  - [7.3 Modo Benchmark](#73-modo-benchmark)
  - [7.4 Integração com o Driver do Marco 2](#74-integração-com-o-driver-do-marco-2)
- [8. Modo de Uso](#8-modo-de-uso)
- [9. Testes e Resultados](#9-testes-e-resultados)
- [10. Referências](#10-referências)

---

## 1. Introdução

Esse repositório é referente ao Marco 3 da disciplina de SD (Sistemas 
Digitais) - TEC499. Depois de dois marcos focados no hardware e na 
comunicação com a FPGA, chegamos na etapa final: desenvolver a aplicação 
em C que o usuário vai usar de verdade para classificar dígitos.

No Marco 1 o co-processador ELM foi implementado na FPGA da DE1-SoC, e 
no Marco 2 nosso grupo desenvolveu o driver em Assembly ARM que faz a 
comunicação entre o HPS e esse hardware via MMIO. Com tudo isso funcionando 
e validado, o Marco 3 fecha o ciclo com uma aplicação completa, que inclui 
interface visual pelo VGA e três formas diferentes de uso.

---

## 2. Sobre o Projeto

O sistema classifica dígitos manuscritos de 0 a 9 usando o modelo ELM 
rodando diretamente no hardware da FPGA. A aplicação desse marco é a parte 
que o usuário enxerga: ela recebe as entradas, prepara os dados e mostra 
o resultado. Por baixo dos panos, quem faz o trabalho pesado ainda é o 
driver do Marco 2, que continua sendo usado do jeito que foi entregue.

Nesse marco aqui é a integração com o IP-Core VGA, disponibilizado para a 
disciplina, que permite tanto mostrar imagens no monitor quanto capturar 
o que o usuário desenha com o mouse para depois mandar ao co-processador.

---

## 3. Declaração do Problema

### 3.1 Contexto

No Marco 2 o driver ficou funcional, com acurácia acima de 80% nos testes, 
mas a inferência só rodava através de um programa de testes simples em C, 
sem nenhuma interface visual e sem flexibilidade para o usuário escolher 
a imagem. Para concluir o projeto de forma completa, era necessário ir além 
disso.

O Marco 3 entra justamente nessa lacuna. O desafio principal foi integrar 
o IP-Core VGA ao projeto já existente sem quebrar o que estava funcionando, 
além de garantir que a imagem exibida no monitor fosse exatamente a mesma 
enviada ao co-processador. O modo de desenho via mouse também trouxe um 
trabalho extra, já que o que o usuário desenha na tela precisa ser 
convertido para o formato que o hardware espera. Por fim, o modo benchmark 
exigiu atenção na medição de tempo para que as métricas calculadas 
refletissem o comportamento real do sistema.

### 3.2 Requisitos Principais

**Integração do IP-Core VGA ao projeto existente**

**Modo de inferência a partir de imagem lida de arquivo**

**Modo de inferência a partir de desenho capturado via mouse e VGA**

**Modo de validação/benchmark com N imagens**

**Interface via linha de comando** (caminho da imagem e parâmetros)

**Exibição da imagem a ser inferida via VGA**

**Comunicação com o co-processador via driver do Marco 2**

**Cálculo de acurácia (%), latência média, desvio padrão e throughput (imagens/s)**

**Log de resultados salvo em CSV**

## 4. Fundamentação Teórica

### 4.1 IP-Core VGA

O IP-Core VGA foi utilizado para permitir a exibição de imagens e 
informações diretamente no monitor conectado à placa DE1-SoC. Esse módulo, 
disponibilizado para a disciplina, é responsável por gerar os sinais de 
vídeo necessários para a saída VGA.

A comunicação entre o HPS e o IP-Core ocorre por meio de MMIO, utilizando 
registradores mapeados em memória. Dessa forma, a aplicação consegue enviar 
pixels individualmente para serem desenhados na tela. Cada pixel é 
representado por uma palavra de 32 bits contendo informações de posição e 
cor. Nesse formato, os bits mais significativos carregam as coordenadas X 
e Y do pixel, enquanto os bits menos significativos definem as componentes 
de cor RGB — com 3 bits para vermelho, 3 para verde e 2 para azul.

Durante o desenvolvimento, o módulo VGA foi utilizado principalmente para 
exibir imagens do conjunto MNIST e permitir a interação do usuário no modo 
de desenho, facilitando a visualização dos dados enviados ao co-processador.

---

### 4.2 Modos de Operação

A aplicação foi desenvolvida com três modos principais de operação, 
permitindo diferentes formas de utilização do sistema.

No modo Arquivo, o usuário informa o caminho de uma imagem do conjunto 
MNIST armazenada em formato binário. A imagem é exibida na tela, enviada 
ao co-processador e o dígito reconhecido é apresentado ao usuário.

No modo Desenho, o usuário pode desenhar manualmente um dígito utilizando 
o mouse. O botão esquerdo é utilizado para desenhar e o botão direito para 
confirmar o desenho. Após a confirmação, o desenho é convertido para o 
formato esperado pela rede neural e enviado para classificação. Antes do 
envio, é aplicado um filtro simples para suavizar os traços e melhorar a 
aproximação com as imagens utilizadas no treinamento.

Já o modo Benchmark realiza a execução automática de um conjunto de 1000 
imagens, exibindo cada uma no monitor durante o processamento. O gabarito 
é fixo — imagens de 0 a 99 correspondem ao dígito 0, de 100 a 199 ao 
dígito 1, e assim sucessivamente até 900 a 999 para o dígito 9. Ao final 
do processo, são calculadas métricas como acurácia, latência e throughput, 
além da geração de um arquivo CSV contendo os resultados obtidos.

---

### 4.3 Métricas de Benchmark

Para avaliar o desempenho do sistema foram utilizadas métricas amplamente 
empregadas em aplicações de classificação de imagens.

A acurácia representa o percentual de imagens classificadas corretamente 
em relação ao total processado, permitindo avaliar a qualidade das 
inferências realizadas pelo co-processador.

A latência corresponde ao tempo necessário para processar uma única imagem, 
sendo medida durante a etapa de inferência. Essa métrica permite verificar 
a velocidade de resposta do sistema.

Além disso, foi calculado o desvio padrão das latências, utilizado para 
analisar a estabilidade do tempo de execução entre diferentes inferências.

Por fim, o throughput indica a quantidade de imagens que podem ser 
classificadas por segundo, fornecendo uma visão geral da capacidade de 
processamento da solução desenvolvida.