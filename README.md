# ChargeGrid Intelligence

**FIAP x GoodWe — EV Challenge 2026**
**Sprint 3: Prototipagem Funcional e Integração**

Turma 1CCR, semestre 2026.1 — Time RM 574085

## Equipe

- Enzo Ricardo Silva, RM 571333
- Eric Hernandes Penhalbell, RM 570237
- Matheus Borges Soares, RM 574085
- Joao Guilherme Figueiredo, RM 572697
- Ryan Luther Roque, RM 572993

## Resumo

O ChargeGrid Intelligence é um Hub Operacional para Eletropostos Comerciais. Esta entrega cobre a Sprint 3 do EV Challenge 2026, que pede um protótipo funcional demonstrando a integração entre os componentes já desenvolvidos: simulação do eletroposto, controle de demanda, tarifação dinâmica, protocolo aberto OCPP, notificação ao usuário e um módulo de histórico com algoritmos de busca, ordenação e estatística.

O núcleo técnico é um programa em C, `chargegrid_sprint3_corrigido.c`, testado e compilado com `gcc -Wall -Wextra` sem nenhum warning. Todo resultado citado neste documento (mensagens OCPP, tarifas, comparações de busca e ordenação, estatísticas) veio de uma execução real desse programa.

## Como compilar e rodar

```bash
gcc -Wall -Wextra -o chargegrid3 chargegrid_sprint3_corrigido.c -lm
./chargegrid3
```

Opcional, para ativar o chatbot e a notificação por WhatsApp:

```bash
export OLLAMA_API_KEY="sua_chave"
export CG_WHATSAPP="seu_telefone"
export CG_WA_APIKEY="sua_chave_callmebot"
```

Sem essas variáveis, o programa roda normalmente e avisa que esses dois módulos estão em modo offline.

## Arquitetura e integração dos componentes

O diagrama abaixo organiza os componentes nas três camadas da arquitetura híbrida definida pela GoodWe: Física, Conectividade e Digital. Cada conexão está classificada por maturidade real: já funcional no código, funcional de forma manual e sem automação, ou ainda pendente no roadmap.


<img width="2179" height="1451" alt="imagem" src="https://github.com/user-attachments/assets/2bcbefe4-a28c-4e93-9135-44d6d9432dff" />


### O que já roda integrado dentro do simulador

O `chargegrid_sprint3_corrigido.c` não é um conjunto de módulos separados: é um único programa em que cada ação dispara a próxima automaticamente.

- Cadastrar uma sessão aciona o controle de demanda (`controle_demanda()`) antes de liberar potência para a vaga.
- O controle de demanda alimenta o motor de tarifação (`calcular_sessao()`), que aplica a sobretaxa de alta demanda quando a rede passa de 80% da capacidade.
- Cada mudança de estado gera uma mensagem OCPP 1.6 real (`BootNotification`, `StatusNotification`, `StartTransaction`, `MeterValues`, `StopTransaction`) e regenera o `dashboard.html`.
- Encerrar a sessão grava o registro no vetor de histórico, dispara a notificação via WhatsApp quando configurada, e libera dados para os algoritmos de busca, ordenação e estatística.

Essa cadeia inteira roda dentro do mesmo binário e não depende de nenhuma peça externa para ser demonstrada em vídeo.

### O que hoje é manual, sem API

- A `base_goodwe.csv` e o notebook de cálculo (L(p)) usam o mesmo vocabulário de variáveis do simulador, mas ainda não leem os dados gerados pelo C de forma automática.
- O chatbot Python do operador (ChargeGrid AI) roda separado do simulador em C. Os dois compartilham vocabulário técnico (OCPP, MODBUS, ISO 15118), mas não trocam dados em tempo real.

### O que ainda é roadmap

- Uma ponte de API real (FastAPI) entre o C e os módulos em Python.
- O módulo de IA preditiva de picos de consumo e o agente Síndico Virtual para o eixo condominial.

## Estruturas de dados e algoritmos implementados

Esta é a parte que melhor conecta o projeto ao conteúdo da disciplina de Estruturas de Dados. Todos os números abaixo vêm de uma execução real do programa, com 12 sessões de demonstração carregadas pela opção 12 do menu.

| Componente | Descrição |
|---|---|
| Vetor de estruturas | `struct RegistroSessao` guarda cada recarga concluída, em um vetor `HistoricoSessoes` de até 100 posições |
| Busca linear por ID | O(n). No teste, encontrar o registro 2 custou 2 comparações |
| Busca binária por ID | O(log n), exige vetor ordenado. No mesmo teste, custou 4 comparações; a vantagem cresce com o tamanho do vetor |
| Busca por usuário/veículo | O(n × m), varredura comparando texto |
| Bubble Sort / Insertion Sort | Implementados manualmente, sem `qsort`. Contam comparações e trocas reais; Bubble Sort tem parada antecipada (melhor caso O(n)) |
| Estatísticas do histórico | Energia total, faturamento, ticket médio, extremos de consumo, distribuição por turno/tipo, e estimativa de CO2 evitado |

O cálculo de CO2 evitado corresponde à ideia de "Relatório de ESG automático" do banco de ideias de expansão do negócio, e já está implementado, não é mais só uma proposta.

## Justificativa técnica das escolhas

- **Um único processo em C para o núcleo físico**: concentrar sessão, demanda e tarifação no mesmo binário elimina a latência de rede justamente na parte do sistema que precisa reagir rápido a uma mudança de carga.
- **OCPP 1.6 simulado em JSON**: é o protocolo aberto exigido pela Resolução Normativa ANEEL nº 1.000/2021 para interoperabilidade de equipamentos não exclusivos.
- **Vetor de estruturas em vez de lista encadeada**: o histórico tem tamanho máximo conhecido (100 sessões), então o acesso direto por índice é mais simples e mais rápido do que uma lista, sem precisar de alocação dinâmica.
- **Tarifação em camadas multiplicativas**: turno, fidelidade, reserva e alta demanda seguem o mesmo tipo de função que o notebook de cálculo diferencial otimiza, permitindo no futuro plugar o preço ótimo calculado no lugar da tarifa-base fixa.
- **Notificação e IA acopladas ao fim da sessão**: evita que a demonstração dependa de infraestrutura de terceiros no ar durante a gravação.

## Resultados funcionais registrados em teste

### Controle de demanda
Teto de rede: 60 kW. Potência mínima por vaga: 3,7 kW. Potência máxima por vaga: 22 kW.

### Tarifação dinâmica
- Turno tarifário: off-peak R$ 0,55/kWh, normal R$ 0,85/kWh, pico R$ 1,20/kWh
- Fidelidade (Premium/Frota): −5% sobre a tarifa
- Reserva antecipada: −5% (≥30 min) ou −10% (≥120 min)
- Alta demanda (rede acima de 80%): +10% de sobretaxa
- Piso de segurança: tarifa final nunca abaixo de R$ 0,10/kWh

### Saída real do log OCPP

```
[OCPP][BootNotification] {"chargePointModel":"GoodWe-AC22","chargePointVendor":"GoodWe"}
[OCPP][StatusNotification] {"connectorId":1,"status":"Charging"}
[OCPP][StartTransaction] {"connectorId":1,"idTag":"usuario01","meterStart":0}
[OCPP][StartTransaction.conf] {"idTagInfo":{"status":"Accepted"},"transactionId":110}
```

### Fundamento matemático da tarifa

Modelo L(p) = −35p² + 298,5p − 406, com 1,5 ≤ p ≤ 6,5, representando o lucro diário em função da tarifa por kWh. Ponto crítico em p* ≈ R$ 4,26/kWh, confirmado como máximo global pela segunda derivada. No piso regulatório (R$ 1,50) o posto opera com prejuízo de R$ 37,00/dia; no teto (R$ 6,50) o lucro cai 75% abaixo do máximo possível; no ponto ótimo o ganho chega a R$ 174,94 a mais por dia do que operar no teto.

### Estatística sobre a base de 120 registros

- Energia consumida por sessão: média de 42,74 kWh, mediana de 42,81 kWh, desvio padrão de 22,25 kWh
- Receita por sessão: média de R$ 38,50, mediana de R$ 37,77, desvio padrão de R$ 20,57

## Conexão com os conteúdos da disciplina

- **Estruturas de dados**: vetor de estruturas, busca linear e binária, structs `Sessao`, `Vaga` e `Eletroposto`.
- **Algoritmos de ordenação e complexidade**: Bubble Sort e Insertion Sort manuais, com contagem real de comparações e trocas, mostrando na prática melhor caso O(n) e pior caso O(n²).
- **Cálculo diferencial**: otimização da tarifa via L'(p) e L''(p), aplicando derivadas e análise de concavidade a um problema real de precificação.
- **Estatística descritiva**: médias, desvio padrão, coeficiente de variação e visualizações sobre a base real de operação.
- **Arquitetura de computadores**: pseudoexecução do eletroposto, com ciclo fetch/decode/execute e uso de MODBUS e OCPP.
- **Redes e protocolos**: simulação de mensagens OCPP 1.6, com base na Resolução Normativa ANEEL nº 1.000/2021.

## Vídeo de demonstração

[\[Link do vídeo no YouTube — não listado\]](https://youtu.be/6gE17DYjLYs)
[\[Link do vídeo no YouTube — não listado\]](https://youtu.be/yZ86Z-Tfa6E)


## Entregáveis desta sprint

Arquivo `.txt` com nome e RM de cada integrante, link do vídeo e link deste repositório, conforme especificado no enunciado da Sprint 3.
