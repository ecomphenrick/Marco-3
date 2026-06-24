# Marco-3
# Projeto Coprocessador - Sistemas Digitais (PBL)

Aplicação final do sistema de classificação de dígitos com interface VGA,
integrando o co-processador ELM desenvolvido nos marcos anteriores.

---

## Sumário

- [1. Introdução](#1-introdução)
  - [1.1. Sobre o Projeto](#11-sobre-o-projeto)
  - [1.2. Contexto](#12-contexto)
  - [1.3. Requisitos Principais](#13-requisitos-principais)
- [2. Fundamentação Teórica](#2-fundamentação-teórica)
  - [2.1. IP-Core VGA](#21-ip-core-vga)
  - [2.2. Modos de Operação](#22-modos-de-operação)
  - [2.3. Métricas de Benchmark](#23-métricas-de-benchmark)
  - [2.4. Protocolo do Mouse Linux](#24-protocolo-do-mouse-linux)
- [3. Materiais e Métodos](#3-materiais-e-métodos)
  - [3.1. Materiais](#31-materiais)
    - [3.1.1. DE1-SoC](#311-de1-soc)
    - [3.1.2. IP-Core VGA](#312-ip-core-vga)
    - [3.1.3. Mouse](#313-mouse)
    - [3.1.4. Co-processador — Marco 1](#314-co-processador--marco-1)
    - [3.1.5. Driver Assembly — Marco 2](#315-driver-assembly--marco-2)
  - [3.2. Metodologia](#32-metodologia)
- [4. Arquitetura e Descrição da Solução](#4-arquitetura-e-descrição-da-solução)
  - [4.1. Visão Geral](#41-visão-geral)
  - [4.2. Modo Arquivo](#42-modo-arquivo)
  - [4.3. Modo Desenho](#43-modo-desenho)
  - [4.4. Modo Benchmark](#44-modo-benchmark)
- [5. Modo de Uso](#5-modo-de-uso)
- [6. Testes](#6-testes)
  - [6.1. Ambiente e Configuração de Teste](#61-ambiente-e-configuração-de-teste)
  - [6.2. Casos de Teste / Cenários](#62-casos-de-teste--cenários)
  - [6.3. Procedimento](#63-procedimento)
- [7. Resultados](#7-resultados)
  - [7.1. Resultados — Modo Arquivo](#71-resultados--modo-arquivo)
  - [7.2. Resultados — Modo Desenho](#72-resultados--modo-desenho)
  - [7.3. Resultados — Modo Benchmark](#73-resultados--modo-benchmark)
  - [7.4. Análise / Discussão dos Resultados](#74-análise--discussão-dos-resultados)
- [8. Referências](#8-referências)

---

## 1. Introdução

Este repositório corresponde ao Marco 3 da disciplina de Sistemas Digitais
(TEC499). Os dois marcos anteriores trataram da implementação em hardware e
da comunicação com a FPGA; esta etapa final entrega a aplicação em C que o
usuário efetivamente utiliza para classificar dígitos.

No Marco 1, o co-processador ELM foi implementado na FPGA da DE1-SoC. No
Marco 2, o driver em Assembly ARM responsável pela comunicação entre o HPS
e o hardware via MMIO foi desenvolvido e validado. O Marco 3 fecha esse
ciclo com uma aplicação completa, incorporando interface visual via VGA e
três modos distintos de utilização.

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

### 1.2. Contexto

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
- **Modo Arquivo** — ler uma imagem em formato `.bin` armazenada em disco e
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

### 2.2. Modos de Operação

A aplicação possui três modos principais:

- **Arquivo** — o usuário informa o caminho de uma imagem MNIST em formato
  binário; a imagem é exibida na tela, enviada ao co-processador, e o
  dígito reconhecido é apresentado ao usuário.
- **Desenho** — o usuário desenha um dígito com o mouse (botão esquerdo
  desenha, botão direito confirma). Após a confirmação, o desenho é
  convertido para o formato esperado pela rede e submetido à classificação,
  passando antes por um filtro de suavização que aproxima o traço do
  padrão das imagens de treinamento.
- **Benchmark** — executa automaticamente a classificação de um conjunto de
  1000 imagens, exibindo cada uma no monitor durante o processamento. O
  gabarito segue uma distribuição fixa por faixas de índice (cada faixa de
  100 imagens corresponde a um dígito, de 0 a 9). Ao final, são calculadas
  métricas de desempenho e gerado um arquivo CSV com os resultados.

### 2.3. Métricas de Benchmark

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

### 2.4. Protocolo do Mouse Linux

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

### 3.1. Materiais

#### 3.1.1. DE1-SoC

A DE1-SoC permanece como a plataforma principal do projeto, combinando um
processador ARM Cortex-A9 dual-core (executando Linux embarcado) com uma
FPGA Cyclone V responsável pelos módulos implementados em hardware.

Em relação ao Marco 2, foram adicionados três novos PIOs para comunicação
com o controlador VGA, mapeados nos offsets `0x30`, `0x40` e `0x50` da
Lightweight Bridge. Um monitor VGA externo e um mouse USB foram conectados
para interação com o sistema, e um novo arquivo `.sof` foi gerado e
gravado na FPGA para refletir essas alterações.

#### 3.1.2. IP-Core VGA

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

#### 3.1.3. Mouse

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

#### 3.1.4. Co-processador — Marco 1

O co-processador utilizado neste projeto foi cedido aos grupos e
implementado pelo monitor da disciplina, Maike de Oliveira. Junto a ele,
foi entregue uma documentação detalhada cobrindo seu modo de uso e
funcionamento interno.

A partir desse material, o grupo iniciou o processo de compreender como o
co-processador funcionava e, principalmente, como ele seria utilizado no
projeto. Ficou estabelecido nas sessões tutoriais que o co-processador
seria tratado como uma caixa-preta — ou seja, sem necessidade de alterar
sua implementação interna —, mas que caberia ao grupo conectá-lo
corretamente ao restante do sistema, já que essa integração entre a FPGA
e o HPS era justamente a base do problema proposto no Marco 2.

Para mais detalhes sobre a implementação interna do co-processador,
recomenda-se consultar o repositório do monitor Maike, indicado na seção
de Referências.

#### 3.1.5. Driver Assembly — Marco 2

O driver Assembly desenvolvido no Marco 2 foi reaproveitado no Marco 3. As
funções de mapeamento de memória (`mapeia_memoria()`), reset do
co-processador (`reset_coprocessador()`) e início da inferência
(`comeca_infer()`) permaneceram inalteradas.

A principal mudança ocorreu nas funções de transferência de dados: elas
deixaram de receber caminhos de arquivo como parâmetro e passaram a
receber diretamente os buffers já carregados em memória — por exemplo,
`store_bias(base, buf_bias)`. Essa mudança permitiu separar o tempo de
pré-processamento e leitura de arquivos do tempo efetivo de comunicação
com o hardware, tornando as medições de desempenho mais precisas.

### 3.2. Metodologia

Este projeto foi desenvolvido por meio da metodologia PBL (Problem Based
Learning), em que o aprendizado é conduzido a partir de um problema real
proposto pelo professor. As atividades são organizadas em sessões
tutoriais, nas quais o grupo assume cargos definidos e cumpre metas
estabelecidas para aquela etapa, e em sessões de desenvolvimento, nas
quais a equipe trabalha na implementação da solução.

A condução do Marco 3 seguiu os seguintes passos:

1. **Entendimento do problema** — compreensão do que precisava ser
   entregue nesta etapa e definição de por onde o desenvolvimento deveria
   seguir.
2. **Priorização do modo Desenho** — após a análise inicial, identificou-se
   que o modo Desenho seria provavelmente o mais complexo de implementar
   entre os três modos, e por isso o trabalho começou por ele.
3. **Conexão dos PIOs e uso inicial do VGA e do mouse** — etapa de
   familiarização prática com os novos periféricos, validando a
   comunicação básica antes de integrá-los à lógica da aplicação.
4. **Implementação do modo Desenho** — com o funcionamento do VGA e do
   mouse compreendido, foram implementadas as funções referentes a esse
   modo (detalhadas na seção [Descrição da Solução](#5-descrição-da-solução)).
5. **Implementação do modo Arquivo** — finalizado o modo Desenho, o
   desenvolvimento avançou para a função de inferência a partir de
   arquivo, incluindo a elaboração da função `png_para_buffer`, que
   estendeu o suporte para além do formato `.bin`, passando a aceitar
   também imagens `.png` (mais detalhes na seção
   [Descrição da Solução](#5-descrição-da-solução)).
6. **Implementação do modo Benchmark** — por fim, foi desenvolvida a
   função do modo Benchmark, considerada a mais simples pelo grupo, já
   que uma lógica semelhante havia sido utilizada no marco anterior.
   A principal adição foi a função de geração do log de resultados em
   CSV.

---

## 4. Arquitetura e Descrição da Solução

### 4.1. Visão Geral

A solução é composta por uma aplicação em C que orquestra três modos de
operação — Arquivo, Desenho e Benchmark —, todos convergindo para o mesmo
fluxo de inferência: carregar os pesos da rede no co-processador, enviar
uma imagem de 28×28 pixels e ler o dígito resultante. O que diferencia os
modos é apenas a origem dessa imagem — um arquivo em disco, um desenho
feito pelo usuário via mouse, ou um lote de imagens processado
automaticamente. As seções seguintes detalham cada um desses modos
individualmente.

### 4.2. Modo Arquivo

O modo Arquivo segue uma lógica de conversão inversa à do modo Desenho: em
vez de comprimir um desenho de 224×224 para uma matriz de 28×28, ele
expande uma imagem já pronta de 28×28 (784 pixels) para a área de 224×224
do VGA, permitindo exibi-la na tela antes da inferência.

O usuário informa o caminho de um arquivo, que pode estar em formato
`.bin` ou `.png`. No caso de um `.bin`, os 784 bytes são lidos diretamente
do disco para o buffer da imagem. No caso de um `.png`, a biblioteca
`stb_image` é utilizada para carregar o arquivo e convertê-lo para um
único canal (escala de cinza), preenchendo o mesmo buffer de 784
posições.

Com a imagem em mãos, a lógica é percorrer cada um dos 784 pixels da
matriz 28×28: para cada pixel, calcula-se a intensidade de cinza
correspondente e desenha-se, no VGA, um quadrado de 8×8 pixels naquela
tonalidade — ocupando a posição equivalente dentro da área de 224×224
reservada na tela. Esse processo de ampliação é o que torna a imagem de
28×28, originalmente pequena, visível e legível ao usuário antes do envio
ao co-processador.

Após a exibição, os pesos da rede são carregados, a imagem é enviada ao
hardware via `store_imagem`, e a inferência é disparada, retornando o
dígito reconhecido.

### 4.3. Modo Desenho

No modo Desenho, a ideia em alto nível foi permitir que o usuário
desenhasse um dígito diretamente na tela usando o mouse, dentro de uma
área de 224×224 pixels reservada no VGA, e depois comprimir esse desenho
para uma matriz de 28×28 — o formato de entrada esperado pela rede
neural.

Essa área de desenho é dividida em uma grade de 28×28 células, cada uma
correspondendo a um bloco de 8×8 pixels na tela. Durante a captura, o
movimento do mouse é lido continuamente do arquivo `/dev/input/mice`, e a
posição do cursor é atualizada com base nos deslocamentos relativos
reportados pelo dispositivo. Enquanto o botão esquerdo está pressionado,
os pixels ao redor do cursor são pintados tanto no canvas interno quanto
na tela; o botão do meio limpa a área de desenho, e o botão direito
finaliza a captura.

Após o usuário finalizar o desenho, o programa percorre cada uma das
28×28 células da grade e verifica se algum pixel dentro daquele bloco de
8×8 foi pintado. Caso tenha sido, a célula correspondente recebe o valor
255 na matriz final; caso contrário, recebe 0. O resultado é, então,
suavizado por um filtro de blur (média dos vizinhos), que aproxima o
traço grosso do mouse da textura mais suave dos dígitos manuscritos do
conjunto de treinamento, antes de ser enviado ao co-processador para
inferência.

Para viabilizar esse modo, foi necessário implementar, além da lógica
própria de captura e conversão, um conjunto de funções de suporte ao VGA:
`vga_reset` (reinicialização do controlador), `vga_clear_screen` (limpeza
da tela) e `vga_draw_pixel` (escrita individual de pixels), todas
utilizadas tanto para desenhar a interface do canvas quanto para
refletir, em tempo real, os traços feitos pelo usuário.

### 4.4. Modo Benchmark

O modo Benchmark automatiza a classificação de um conjunto de imagens
para avaliar o desempenho do sistema como um todo. Ele espera encontrar,
na pasta `bins/`, arquivos numerados de `0.bin` a `999.bin`, totalizando
1000 imagens.

O gabarito de cada imagem não é lido de um arquivo separado, mas sim
derivado diretamente do índice numérico do arquivo: a divisão inteira do
número da imagem por 100 determina o dígito correto esperado. Dessa
forma, as imagens de índice 0 a 99 correspondem ao dígito 0, de 100 a 199
ao dígito 1, e assim por diante até 900 a 999 corresponderem ao dígito 9
— resultando em 100 imagens para cada um dos 10 dígitos.

Para cada imagem do conjunto, o programa carrega o arquivo
correspondente, envia-o ao co-processador e mede o tempo de inferência
usando `clock_gettime`, comparando o dígito inferido com o dígito
esperado (obtido pelo índice) para contabilizar acertos e erros, tanto de
forma geral quanto por classe. Ao final do processamento, são calculadas
as métricas de desempenho — acurácia, latência média, desvio padrão e
throughput — e os resultados são salvos em arquivos CSV (`resultado.csv`
com o detalhamento por imagem, e `metricas_gerais.csv` com o resumo geral
e por classe).

---

## 5. Modo de Uso

**Passo 1:** Conecte a placa DE1-SoC ao computador via cabo USB-Blaster
(para programação da FPGA) e via Ethernet ou USB-serial (para acesso via
SSH ao Linux embarcado).

**Passo 2:** Faça o download do projeto e, via SSH, transfira a pasta
`Aplicação` para a placa.

**Passo 3:** Abra a pasta `Coprocessador` no Quartus, compile o projeto e
programe o arquivo `.sof` gerado na placa.

**Passo 4:** No terminal da placa, dentro da pasta do projeto, execute
`make` para compilar o executável.

**Passo 5:** Em seguida, execute o programa com `sudo ./exe` (ou apenas
`./exe`, dependendo das permissões de acesso ao `/dev/input/mice` e à
memória mapeada).

**Passo 6:** No menu exibido, escolha o modo desejado — Arquivo, Desenho
ou Benchmark — digitando o número correspondente e pressionando Enter.

---

## 6. Testes

### 6.1. Ambiente e Configuração de Teste

*(seção a ser escrita)*

### 6.2. Casos de Teste / Cenários

*(seção a ser escrita)*

### 6.3. Procedimento

*(seção a ser escrita)*

---

## 7. Resultados

### 7.1. Resultados — Modo Arquivo

*(seção a ser escrita)*

### 7.2. Resultados — Modo Desenho

*(seção a ser escrita)*

### 7.3. Resultados — Modo Benchmark

*(seção a ser escrita)*

### 7.4. Análise / Discussão dos Resultados

*(seção a ser escrita)*

---

## 8. Referências

*(seção a ser escrita)*
