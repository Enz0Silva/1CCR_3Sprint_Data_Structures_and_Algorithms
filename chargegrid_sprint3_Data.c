/*
 * ================================================================
 *  ChargeGrid Intelligence — Sprint 3
 *  EV Challenge 2026 | FIAP x GoodWe
 *  Data Structures and Algorithms
 *
 *  ================================================================
 *  ESTRUTURAS DE DADOS E ALGORITMOS (requisitos da Sprint 3)
 *  ================================================================
 *
 *  A. VETOR DE ESTRUTURAS
 *     A struct RegistroSessao (typedef) guarda o resumo de uma
 *     recarga concluida. Todas ficam no vetor HistoricoSessoes,
 *     com controle de quantas posicoes estao ocupadas.
 *
 *  B. BUSCA
 *     - Busca Linear por ID .............. O(n)
 *     - Busca Binaria por ID ............. O(log n)
 *     - Busca Linear por usuario ......... O(n * m)
 *     O programa conta as comparacoes reais para comprovar a
 *     diferenca de complexidade na pratica.
 *
 *  C. ORDENACAO (implementada manualmente, sem qsort)
 *     - Bubble Sort com parada antecipada ... O(n^2) / O(n) melhor caso
 *     - Insertion Sort ...................... O(n^2) / O(n) melhor caso
 *     Criterios: ID, energia, custo ou tempo, crescente/decrescente.
 *
 *  D. ESTATISTICAS
 *     Total de sessoes, energia fornecida, faturamento, ticket
 *     medio, maior e menor consumo, distribuicao por turno.
 *
 *  ================================================================
 *  FUNCIONALIDADES HERDADAS DAS SPRINTS ANTERIORES
 *  ================================================================
 *
 *  1. DASHBOARD WEB (dashboard.html)
 *     O programa gera um arquivo HTML que atualiza sozinho
 *     a cada 5 segundos mostrando todas as vagas, demanda
 *     e status em tempo real no navegador.
 *
 *  2. PREVISÃO DE FILA
 *     Quando todas as vagas estão ocupadas, o sistema calcula
 *     qual vaga vai liberar primeiro e em quanto tempo,
 *     mostrando isso para o próximo usuário.
 *
 *  3. NOTIFICAÇÃO WHATSAPP
 *     Quando uma sessão é encerrada, o sistema envia uma
 *     mensagem automática via WhatsApp para o número
 *     cadastrado, com o resumo da recarga.
 *     Usa a API gratuita Callmebot (sem cadastro de empresa).
 *
 *  ================================================================
 *  CORRECOES APLICADAS NESTA VERSAO
 *  ================================================================
 *
 *  1. VAGAS NUNCA VOLTAVAM A FICAR DISPONIVEIS
 *     encerrar_sessao_vaga() marcava a vaga como VAGA_CONCLUIDA, mas
 *     encontrar_vaga_livre() so aceitava VAGA_LIVRE. Depois de 5
 *     recargas encerradas a estacao travava em "todas as vagas
 *     ocupadas" e nenhuma sessao nova podia ser cadastrada.
 *     Corrigido em encontrar_vaga_livre() e contar_vagas_livres().
 *
 *  2. LACO INFINITO NO FIM DA ENTRADA (EOF)
 *     ler_int() e ler_float() usavam "while (getchar() != '\n');".
 *     Com a entrada esgotada, getchar() devolve EOF para sempre e o
 *     programa travava. Criada a funcao descartar_linha(), que testa
 *     EOF e encerra a leitura com o valor padrao.
 *
 *  3. ID COMPARADO COMO FLOAT NA ORDENACAO
 *     comparar_registros() convertia a chave inteira 'id' para float.
 *     Agora o ID e comparado como int, de forma exata.
 *
 *  4. DIVISAO POR ZERO NA TARIFA MEDIA
 *     mostrar_estatisticas() dividia por energia_total sem verificar
 *     se ela era zero, o que imprimiria "inf" ou "nan".
 *
 *  5. ROTULO DE COMPLEXIDADE INCORRETO
 *     busca_linear_usuario() imprimia O(n) sendo O(n*m).
 *
 *  Compilacao:
 *    gcc -Wall -Wextra -o chargegrid3 chargegrid_sprint3.c -lm
 *
 *  Configuracoes opcionais:
 *    Windows PowerShell:
 *    $env:OLLAMA_API_KEY="sua_chave_sprint2_data"  (chatbot IA)
 *    $env:CG_WHATSAPP="5511999999999"               (numero com DDI+DDD)
 *    $env:CG_WA_APIKEY="sua_chave_callmebot"        (chave callmebot)
 *
 *  Como ativar WhatsApp (GRATIS):
 *    1. Adicione +34 644 44 74 47 nos seus contatos
 *    2. Mande "I allow callmebot to send me messages"
 *    3. Voce recebe sua API key por WhatsApp
 *    4. export CG_WHATSAPP="5511SEU_NUMERO"
 *    5. export CG_WA_APIKEY="SUA_CHAVE"
 * ================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #define popen  _popen
    #define pclose _pclose
#else
    #include <unistd.h>
#endif

/* ================================================================
 *  CONSTANTES DO SISTEMA
 * ================================================================ */
#define POTENCIA_MAX_KW        22.0f
#define POTENCIA_MIN_KW         3.7f
#define DEMANDA_MAX_TOTAL_KW   60.0f
#define MAX_VAGAS               5

#define TARIFA_NORMAL           0.85f
#define TARIFA_PICO             1.20f
#define TARIFA_OFFPEAK          0.55f
#define DESCONTO_FIDELIDADE     0.05f
#define DESCONTO_RESERVA_MED    0.05f
#define DESCONTO_RESERVA_ALT    0.10f
#define DESCONTO_ALTA_DEMANDA  -0.10f

#define MAX_TENTATIVAS          3
#define MAX_HISTORICO          20
#define MAX_MSG_LEN           2000
#define MAX_RESP_LEN          8192
#define MAX_JSON_LEN         16384
#define MAX_CMD_LEN          20480
#define MAX_LOG               200

#define DASHBOARD_FILE  "dashboard.html"

/* Sprint 3 — tamanho do vetor de estruturas do historico */
#define MAX_SESSOES           100

#define SEP  "================================================================"
#define LINE "----------------------------------------------------------------"

/* ================================================================
 *  ESTRUTURAS DE DADOS
 * ================================================================ */

typedef enum { USUARIO_COMUM = 1, USUARIO_PREMIUM = 2, USUARIO_FROTA = 3 } TipoUsuario;
typedef enum { TURNO_OFFPEAK = 1, TURNO_NORMAL = 2, TURNO_PICO = 3      } TurnoTarifario;

typedef enum {
    VAGA_LIVRE     = 0,
    VAGA_OCUPADA   = 1,
    VAGA_CONCLUIDA = 2
} StatusVaga;

typedef struct {
    char           id_usuario[20];
    char           telefone_whatsapp[20]; /* NOVO Sprint 3 */
    TipoUsuario    tipo_usuario;
    char           modelo_veiculo[50];
    float          capacidade_bateria_kwh;
    float          pct_bateria_atual;
    float          pct_bateria_alvo;
    float          potencia_kw;
    float          potencia_real_kw;
    int            hora_inicio;
    int            minutos_reserva;
    TurnoTarifario turno;
    time_t         timestamp_inicio;     /* NOVO Sprint 3 — hora real de início */
} Sessao;

typedef struct {
    float energia_kwh;
    float tempo_min;
    float tarifa_base;
    float tarifa_final;
    float desconto_pct;
    float custo_reais;
    int   tem_fidelidade;
    int   tem_reserva;
    int   teve_reducao_potencia;
    char  descricao_turno[60];
} ResultadoSessao;

typedef struct {
    int             numero_vaga;
    StatusVaga      status;
    Sessao          sessao;
    ResultadoSessao resultado;
} Vaga;

/* ================================================================
 *  SPRINT 3 — REGISTRO HISTORICO DE UMA SESSAO
 *
 *  Esta e a struct central exigida pelo enunciado. Enquanto a
 *  struct Vaga representa um carregador FISICO (estado atual),
 *  a RegistroSessao representa uma recarga JA CONCLUIDA, guardada
 *  no historico para ser listada, buscada, ordenada e analisada.
 *
 *  O campo 'id' e a chave usada pelos algoritmos de busca.
 * ================================================================ */
typedef struct {
    int            id;                  /* chave de busca (1, 2, 3, ...)   */
    char           id_usuario[20];      /* quem fez a recarga              */
    char           modelo_veiculo[50];  /* modelo do veiculo eletrico      */
    TipoUsuario    tipo_usuario;        /* Comum / Premium / Frota         */
    TurnoTarifario turno;               /* Off-peak / Normal / Pico        */
    int            numero_vaga;         /* em qual carregador ocorreu      */
    int            hora_inicio;         /* hora simulada de inicio (0-23)  */
    float          energia_kwh;         /* energia transferida             */
    float          tempo_min;           /* duracao da recarga              */
    float          potencia_kw;         /* potencia real aplicada          */
    float          tarifa_final;        /* R$/kWh apos descontos           */
    float          custo_reais;         /* valor pago pela sessao          */
    char           data_hora[24];       /* carimbo de tempo real           */
} RegistroSessao;

/* ================================================================
 *  SPRINT 3 — VETOR DE ESTRUTURAS
 *
 *  'itens'  = o vetor de estruturas propriamente dito
 *  'total'  = quantas posicoes estao efetivamente ocupadas
 *             (nunca percorremos alem disso)
 *  'proximo_id' = gerador sequencial de IDs
 *  'ordenado_por_id' = 1 quando o vetor esta em ordem crescente
 *             de ID. A busca binaria SO pode rodar quando esse
 *             campo vale 1 (pre-requisito do algoritmo).
 * ================================================================ */
typedef struct {
    RegistroSessao itens[MAX_SESSOES];
    int            total;
    int            proximo_id;
    int            ordenado_por_id;
} HistoricoSessoes;

/* Criterio escolhido pelo usuario na hora de ordenar */
typedef enum {
    ORD_ID      = 1,
    ORD_ENERGIA = 2,
    ORD_CUSTO   = 3,
    ORD_TEMPO   = 4
} CriterioOrdenacao;

/* ================================================================
 *  SPRINT 3 — RESUMO ESTATISTICO DO HISTORICO
 *
 *  Guarda o resultado de UMA varredura O(n) do vetor. Tanto o
 *  relatorio do terminal quanto o dashboard HTML consomem esta
 *  mesma struct, em vez de cada um percorrer o vetor por conta
 *  propria com o calculo repetido.
 * ================================================================ */
typedef struct {
    int   total_sessoes;
    float energia_total;
    float faturamento;
    float tempo_total;
    float maior_consumo;
    float menor_consumo;
    int   indice_maior;      /* posicao da sessao de maior consumo */
    int   indice_menor;      /* posicao da sessao de menor consumo */
    int   por_turno[4];      /* [1]=off-peak [2]=normal [3]=pico   */
    int   por_tipo[4];       /* [1]=comum    [2]=premium [3]=frota */
} ResumoHistorico;

typedef struct {
    char linhas[MAX_LOG][200];
    int  total;
} LogOCPP;

typedef struct {
    char role[16];
    char content[MAX_MSG_LEN];
} MensagemChat;

typedef struct {
    MensagemChat mensagens[MAX_HISTORICO];
    int          total;
} HistoricoChat;

typedef struct {
    Vaga             vagas[MAX_VAGAS];
    LogOCPP          log;
    HistoricoSessoes historico;   /* NOVO Sprint 3 — vetor de estruturas */
    int              total_sessoes_ativas;
    float            demanda_atual_kw;
    int              hora_sistema;
} Eletroposto;

/* ================================================================
 *  PROTÓTIPOS
 * ================================================================ */
/* Simulador */
void            exibir_banner(void);
void            sep(int tipo);
Sessao          coletar_dados_sessao(const Eletroposto *ep);
ResultadoSessao calcular_sessao(Sessao *s, const Eletroposto *ep);
void            simular_progresso(const Sessao *s, const ResultadoSessao *r);
void            exibir_relatorio(const Sessao *s, const ResultadoSessao *r);
TipoUsuario     menu_tipo_usuario(void);
TurnoTarifario  definir_turno(int hora);
float           ler_float(const char *msg, float min, float max);
int             ler_int(const char *msg, int min, int max);
int             descartar_linha(void);
const char     *str_tipo(TipoUsuario t);
const char     *str_turno(TurnoTarifario t);
const char     *str_status(StatusVaga s);
const char     *str_status_html(StatusVaga s); /* NOVO */

/* Vagas */
void  inicializar_eletroposto(Eletroposto *ep);
int   encontrar_vaga_livre(const Eletroposto *ep);
void  iniciar_sessao_vaga(Eletroposto *ep, int idx);
void  encerrar_sessao_vaga(Eletroposto *ep, int idx);
void  exibir_painel_vagas(const Eletroposto *ep);
void  exibir_relatorio_consolidado(const Eletroposto *ep);
float calcular_demanda_total(const Eletroposto *ep);
float controle_demanda(float pot, const Eletroposto *ep);

/* ================================================================
 *  SPRINT 3 — ESTRUTURAS DE DADOS E ALGORITMOS
 * ================================================================ */
/* Cadastro e listagem */
void inicializar_historico(HistoricoSessoes *h);
int  cadastrar_sessao(HistoricoSessoes *h, const Sessao *s,
                      const ResultadoSessao *r, int numero_vaga);
void listar_sessoes(const HistoricoSessoes *h);
void exibir_detalhe_sessao(const RegistroSessao *g);
void gerar_sessoes_demo(HistoricoSessoes *h);

/* Busca */
int  busca_linear_id(const HistoricoSessoes *h, int id, long *comparacoes);
int  busca_binaria_id(const HistoricoSessoes *h, int id, long *comparacoes);
void busca_linear_usuario(const HistoricoSessoes *h, const char *termo);
void menu_buscar_sessao(HistoricoSessoes *h);

/* Ordenação (implementada manualmente) */
int  comparar_registros(const RegistroSessao *a, const RegistroSessao *b,
                        CriterioOrdenacao crit, int decrescente);
void bubble_sort_sessoes(HistoricoSessoes *h, CriterioOrdenacao crit,
                         int decrescente, long *comparacoes, long *trocas);
void insertion_sort_sessoes(HistoricoSessoes *h, CriterioOrdenacao crit,
                            int decrescente, long *comparacoes, long *trocas);
void menu_ordenar_sessoes(HistoricoSessoes *h);
const char *str_criterio(CriterioOrdenacao c);

/* Estatísticas */
void mostrar_estatisticas(const HistoricoSessoes *h);
void calcular_resumo_historico(const HistoricoSessoes *h, ResumoHistorico *res);

/* ----------------------------------------------------------------
 *  Funcoes de apoio criadas para eliminar codigo repetido.
 *  Cada regra do sistema passa a existir em UM unico lugar:
 *  se a tarifa de pico mudar, muda so dentro de tarifa_do_turno().
 * ---------------------------------------------------------------- */
float       tarifa_do_turno(TurnoTarifario t);
const char *str_turno_detalhado(TurnoTarifario t);
float       desconto_fidelidade(TipoUsuario t);
float       energia_necessaria_kwh(const Sessao *s);
float       tempo_recarga_min(float energia_kwh, float potencia_kw);
int         contar_vagas_livres(const Eletroposto *ep);
const char *str_status_label(StatusVaga s);
void        nome_arquivo_temp(const char *prefixo, char *dest, int tam);

/* Auxiliares */
void ler_texto(const char *msg, char *dest, int tam);
int  contem_texto(const char *texto, const char *busca);

/* NOVIDADE 2 — Previsão de fila */
float calcular_tempo_restante_min(const Vaga *v);
void  exibir_previsao_fila(const Eletroposto *ep);

/* NOVIDADE 1 — Dashboard HTML */
void  gerar_dashboard(const Eletroposto *ep);
void  escapar_html(const char *entrada, char *saida, int max);

/* NOVIDADE 3 — WhatsApp */
void  notificar_whatsapp(const Sessao *s, const ResultadoSessao *r);
void  url_encode(const char *entrada, char *saida, int max);

/* OCPP */
void  ocpp_log(LogOCPP *log, const char *tipo, const char *mensagem);
void  ocpp_boot_notification(Eletroposto *ep);
void  ocpp_status_notification(Eletroposto *ep, int vaga, StatusVaga status);
void  ocpp_start_transaction(Eletroposto *ep, int vaga, const Sessao *s);
void  ocpp_stop_transaction(Eletroposto *ep, int vaga, const ResultadoSessao *r);
void  ocpp_meter_values(Eletroposto *ep, int vaga, float kwh, float pct);
void  exibir_log_ocpp(const LogOCPP *log);

/* Menu */
void  menu_principal(Eletroposto *ep);
void  menu_sessao_nova(Eletroposto *ep);
void  menu_encerrar_sessao(Eletroposto *ep);
void  menu_chatbot(Eletroposto *ep);

/* Chatbot */
void  sessao_para_json(const Sessao *s, const ResultadoSessao *r, char *buf, int sz);
void  construir_system_prompt(const char *json, char *buf, int sz);
int   chamar_api_claude(const char *sp, const HistoricoChat *h, char *rb, int sz);
void  extrair_resposta_json(const char *raw, char *buf, int sz);
void  adicionar_mensagem(HistoricoChat *h, const char *role, const char *content);
void  loop_chatbot(const Sessao *s, const ResultadoSessao *r);
void  escapar_json(const char *in, char *out, int max);
void  imprimir_resposta(const char *texto);

/* ================================================================
 *  MAIN
 * ================================================================ */
int main(void) {
    Eletroposto ep;

    /* Configura UTF-8 no terminal do Windows */
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    inicializar_eletroposto(&ep);
    exibir_banner();

    const char *api_key = getenv("OLLAMA_API_KEY");
    if (!api_key || strlen(api_key) < 10)
        printf("  AVISO: OLLAMA_API_KEY nao configurada. Chatbot em modo offline.\n"
               "  Use: $env:OLLAMA_API_KEY=\"sua_chave_sprint2_data\"\n\n");

    const char *wa_num = getenv("CG_WHATSAPP");
    const char *wa_key = getenv("CG_WA_APIKEY");
    if (!wa_num || !wa_key)
        printf("  AVISO: WhatsApp nao configurado (CG_WHATSAPP / CG_WA_APIKEY).\n\n");

    ocpp_boot_notification(&ep);
    gerar_dashboard(&ep); /* gera dashboard inicial */

    menu_principal(&ep);

    printf("\n  Obrigado por usar o ChargeGrid Intelligence!\n");
    sep(1);
    return 0;
}

/* ================================================================
 *  NOVIDADE 1 — DASHBOARD HTML
 *
 *  Gera um arquivo dashboard.html que:
 *  - Tem uma pagina visual com todas as vagas
 *  - Usa CSS para colorir verde/amarelo/vermelho por status
 *  - Usa JavaScript para recarregar sozinha a cada 5 segundos
 *  - O programa regenera o arquivo a cada mudanca de estado
 *  Basta abrir o arquivo no navegador e deixar aberto!
 * ================================================================ */
void gerar_dashboard(const Eletroposto *ep) {
    FILE *f = fopen(DASHBOARD_FILE, "w");
    if (!f) return;

    /* Calcula totais para o resumo */
    float total_energia = 0.0f, total_custo = 0.0f;
    int   sessoes_ativas = 0, i;
    for (i = 0; i < MAX_VAGAS; i++) {
        if (ep->vagas[i].status != VAGA_LIVRE) {
            total_energia += ep->vagas[i].resultado.energia_kwh;
            total_custo   += ep->vagas[i].resultado.custo_reais;
        }
        if (ep->vagas[i].status == VAGA_OCUPADA) sessoes_ativas++;
    }

    float pct_demanda = (ep->demanda_atual_kw / DEMANDA_MAX_TOTAL_KW) * 100.0f;
    /* Cor da barra de demanda muda conforme o nivel */
    const char *cor_demanda = pct_demanda > 80.0f ? "#e74c3c" :
                              pct_demanda > 50.0f ? "#f39c12" : "#27ae60";

    /* Hora atual do sistema */
    time_t agora = time(NULL);
    char hora_str[64];
    strftime(hora_str, sizeof(hora_str), "%d/%m/%Y %H:%M:%S", localtime(&agora));

    /* ---- Cabeçalho HTML ---- */
    fprintf(f,
        "<!DOCTYPE html>\n"
        "<html lang='pt-BR'>\n"
        "<head>\n"
        "  <meta charset='UTF-8'>\n"
        "  <meta http-equiv='refresh' content='5'>\n"  /* Recarrega a cada 5s */
        "  <title>ChargeGrid — Dashboard</title>\n"
        "  <style>\n"
        "    * { box-sizing: border-box; margin: 0; padding: 0; }\n"
        "    body { font-family: 'Segoe UI', Arial, sans-serif;\n"
        "           background: #0d1117; color: #e6edf3; min-height: 100vh; }\n"
        "    .header { background: #161b22; border-bottom: 2px solid #30363d;\n"
        "              padding: 20px 30px; display:flex;\n"
        "              justify-content:space-between; align-items:center; }\n"
        "    .header h1 { font-size: 1.6rem; color: #58a6ff; }\n"
        "    .header h1 span { color: #3fb950; }\n"
        "    .badge { background:#21262d; border:1px solid #30363d;\n"
        "             padding:6px 14px; border-radius:20px;\n"
        "             font-size:0.82rem; color:#8b949e; }\n"
        "    .badge b { color:#e6edf3; }\n"
        "    .container { padding: 24px 30px; }\n"
        "    /* Cards de resumo no topo */\n"
        "    .cards { display:grid; grid-template-columns:repeat(4,1fr);\n"
        "             gap:16px; margin-bottom:24px; }\n"
        "    .card { background:#161b22; border:1px solid #30363d;\n"
        "            border-radius:10px; padding:18px 20px; }\n"
        "    .card .label { font-size:0.75rem; color:#8b949e;\n"
        "                   text-transform:uppercase; letter-spacing:1px; }\n"
        "    .card .value { font-size:2rem; font-weight:700;\n"
        "                   margin-top:6px; color:#58a6ff; }\n"
        "    .card .sub   { font-size:0.8rem; color:#8b949e; margin-top:4px; }\n"
        "    /* Barra de demanda */\n"
        "    .demanda-box { background:#161b22; border:1px solid #30363d;\n"
        "                   border-radius:10px; padding:18px 20px;\n"
        "                   margin-bottom:24px; }\n"
        "    .demanda-box h3 { font-size:0.85rem; color:#8b949e;\n"
        "                      text-transform:uppercase; margin-bottom:10px; }\n"
        "    .bar-bg { background:#21262d; border-radius:8px; height:22px;\n"
        "              overflow:hidden; }\n"
        "    .bar-fill { height:100%%; border-radius:8px;\n"
        "                background:%s; transition:width 0.5s;\n"
        "                display:flex; align-items:center;\n"
        "                justify-content:center; font-size:0.8rem;\n"
        "                font-weight:700; color:#fff;\n"
        "                width:%.0f%%; min-width:30px; }\n"
        "    /* Grid de vagas */\n"
        "    .vagas-grid { display:grid;\n"
        "                  grid-template-columns:repeat(auto-fit,minmax(280px,1fr));\n"
        "                  gap:16px; }\n"
        "    .vaga { background:#161b22; border:1px solid #30363d;\n"
        "            border-radius:10px; padding:20px; position:relative;\n"
        "            transition: border-color 0.3s; }\n"
        "    .vaga.ocupada  { border-color:#f39c12; }\n"
        "    .vaga.livre    { border-color:#30363d; }\n"
        "    .vaga.concluida{ border-color:#27ae60; }\n"
        "    .vaga-num { font-size:0.72rem; color:#8b949e;\n"
        "                text-transform:uppercase; letter-spacing:1px; }\n"
        "    .vaga-status { display:inline-block; padding:3px 10px;\n"
        "                   border-radius:12px; font-size:0.75rem;\n"
        "                   font-weight:700; margin-bottom:10px; }\n"
        "    .vaga-status.ocupada  { background:#4d3319; color:#f39c12; }\n"
        "    .vaga-status.livre    { background:#1c2128; color:#8b949e; }\n"
        "    .vaga-status.concluida{ background:#1a3a27; color:#3fb950; }\n"
        "    .vaga-veiculo { font-size:1.05rem; font-weight:600;\n"
        "                    color:#e6edf3; margin-bottom:4px; }\n"
        "    .vaga-user  { font-size:0.82rem; color:#8b949e; margin-bottom:12px; }\n"
        "    .vaga-stats { display:grid; grid-template-columns:1fr 1fr;\n"
        "                  gap:8px; }\n"
        "    .stat { background:#0d1117; border-radius:6px; padding:8px 10px; }\n"
        "    .stat .sl { font-size:0.68rem; color:#8b949e; }\n"
        "    .stat .sv { font-size:0.95rem; font-weight:600; color:#58a6ff; }\n"
        "    .alerta { background:#4d1919; color:#ff7b72;\n"
        "              border-radius:6px; padding:6px 10px;\n"
        "              font-size:0.78rem; margin-top:10px; }\n"
        "    .fila-box { background:#1c2128; border:1px solid #f39c12;\n"
        "                border-radius:8px; padding:10px 14px;\n"
        "                margin-top:10px; color:#f39c12;\n"
        "                font-size:0.82rem; }\n"
        "    .footer { text-align:center; padding:20px;\n"
        "              color:#484f58; font-size:0.78rem; }\n"
        "    .pulse { animation: pulse 2s infinite; }\n"
        "    @keyframes pulse {\n"
        "      0%%,100%% { opacity:1; } 50%% { opacity:0.5; } }\n"
        "  </style>\n"
        "</head>\n"
        "<body>\n",
        cor_demanda, pct_demanda > 100.0f ? 100.0f : pct_demanda
    );

    /* ---- Header ---- */
    fprintf(f,
        "<div class='header'>\n"
        "  <h1>⚡ ChargeGrid <span>Intelligence</span></h1>\n"
        "  <div style='display:flex;gap:10px;flex-wrap:wrap;'>\n"
        "    <span class='badge'>🕐 Atualizado: <b>%s</b></span>\n"
        "    <span class='badge'>📡 OCPP 1.6 ativo</span>\n"
        "    <span class='badge'>⏱ Recarrega em <b>5s</b></span>\n"
        "  </div>\n"
        "</div>\n"
        "<div class='container'>\n",
        hora_str
    );

    /* ---- Cards de resumo ---- */
    int livres = contar_vagas_livres(ep);

    fprintf(f,
        "<div class='cards'>\n"
        "  <div class='card'>\n"
        "    <div class='label'>Vagas Livres</div>\n"
        "    <div class='value' style='color:#3fb950'>%d/%d</div>\n"
        "    <div class='sub'>disponíveis agora</div>\n"
        "  </div>\n"
        "  <div class='card'>\n"
        "    <div class='label'>Sessões Ativas</div>\n"
        "    <div class='value' style='color:#f39c12'>%d</div>\n"
        "    <div class='sub'>veículos carregando</div>\n"
        "  </div>\n"
        "  <div class='card'>\n"
        "    <div class='label'>Energia Fornecida</div>\n"
        "    <div class='value'>%.1f</div>\n"
        "    <div class='sub'>kWh no total</div>\n"
        "  </div>\n"
        "  <div class='card'>\n"
        "    <div class='label'>Faturamento</div>\n"
        "    <div class='value' style='color:#3fb950'>R$%.0f</div>\n"
        "    <div class='sub'>arrecadado</div>\n"
        "  </div>\n"
        "</div>\n",
        livres, MAX_VAGAS, sessoes_ativas, total_energia, total_custo
    );

    /* ---- Barra de demanda ---- */
    fprintf(f,
        "<div class='demanda-box'>\n"
        "  <h3>⚡ Carga da Rede Elétrica — %.1f kW / %.0f kW</h3>\n"
        "  <div class='bar-bg'>\n"
        "    <div class='bar-fill'>%.0f%%</div>\n"
        "  </div>\n"
        "</div>\n",
        ep->demanda_atual_kw, DEMANDA_MAX_TOTAL_KW, pct_demanda
    );

    /* ---- Sprint 3: resumo do historico de sessoes ---- */
    if (ep->historico.total > 0) {
        ResumoHistorico res;
        calcular_resumo_historico(&ep->historico, &res);

        fprintf(f,
            "<div class='demanda-box'>\n"
            "  <h3>📊 Historico de Sessoes — %d de %d posicoes do vetor</h3>\n"
            "  <div class='vaga-stats' style='grid-template-columns:repeat(auto-fit,minmax(150px,1fr));'>\n"
            "    <div class='stat'><div class='sl'>Sessoes realizadas</div>"
            "      <div class='sv'>%d</div></div>\n"
            "    <div class='stat'><div class='sl'>Energia fornecida</div>"
            "      <div class='sv'>%.2f kWh</div></div>\n"
            "    <div class='stat'><div class='sl'>Faturamento</div>"
            "      <div class='sv'>R$ %.2f</div></div>\n"
            "    <div class='stat'><div class='sl'>Ticket medio</div>"
            "      <div class='sv'>R$ %.2f</div></div>\n"
            "    <div class='stat'><div class='sl'>Maior consumo</div>"
            "      <div class='sv'>%.2f kWh</div></div>\n"
            "    <div class='stat'><div class='sl'>Menor consumo</div>"
            "      <div class='sv'>%.2f kWh</div></div>\n"
            "  </div>\n"
            "</div>\n",
            res.total_sessoes, MAX_SESSOES,
            res.total_sessoes,
            res.energia_total,
            res.faturamento,
            res.faturamento / res.total_sessoes,
            res.maior_consumo,
            res.menor_consumo
        );
    }

    /* ---- Grid de vagas ---- */
    fprintf(f, "<div class='vagas-grid'>\n");

    for (i = 0; i < MAX_VAGAS; i++) {
        const Vaga *v = &ep->vagas[i];
        /* Antes eram dois ternarios encadeados escritos aqui dentro.
           Agora reusam as funcoes de conversao de status. */
        const char *css_status   = str_status_html(v->status);
        const char *label_status = str_status_label(v->status);

        char veiculo_esc[100], user_esc[40];
        escapar_html(v->sessao.modelo_veiculo, veiculo_esc, sizeof(veiculo_esc));
        escapar_html(v->sessao.id_usuario,     user_esc,    sizeof(user_esc));

        fprintf(f,
            "  <div class='vaga %s'>\n"
            "    <div class='vaga-num'>VAGA %d</div>\n"
            "    <span class='vaga-status %s'>%s</span>\n",
            css_status, v->numero_vaga, css_status, label_status
        );

        if (v->status == VAGA_OCUPADA) {
            /* Calcula progresso real */
            float restante = calcular_tempo_restante_min(v);
            float total    = v->resultado.tempo_min;
            float decorrido = total - restante;
            float progresso = total > 0.0f ? (decorrido / total) * 100.0f : 0.0f;
            if (progresso > 100.0f) progresso = 100.0f;
            if (progresso < 0.0f)  progresso = 0.0f;

            fprintf(f,
                "    <div class='vaga-veiculo'>%s</div>\n"
                "    <div class='vaga-user'>👤 %s</div>\n"
                /* Barra de progresso da bateria */
                "    <div style='margin-bottom:12px;'>\n"
                "      <div style='font-size:0.72rem;color:#8b949e;margin-bottom:4px;'>"
                "        Progresso da recarga</div>\n"
                "      <div class='bar-bg' style='height:12px;'>\n"
                "        <div class='bar-fill' style='background:#58a6ff;height:12px;"
                "             font-size:0.65rem;width:%.0f%%;min-width:0;'>%.0f%%</div>\n"
                "      </div>\n"
                "    </div>\n"
                "    <div class='vaga-stats'>\n"
                "      <div class='stat'><div class='sl'>Potência</div>"
                "        <div class='sv'>%.1f kW</div></div>\n"
                "      <div class='stat'><div class='sl'>Energia</div>"
                "        <div class='sv'>%.2f kWh</div></div>\n"
                "      <div class='stat'><div class='sl'>Custo atual</div>"
                "        <div class='sv'>R$ %.2f</div></div>\n"
                "      <div class='stat'><div class='sl'>Resta</div>"
                "        <div class='sv'>~%.0f min</div></div>\n"
                "    </div>\n",
                veiculo_esc, user_esc,
                progresso, progresso,
                v->sessao.potencia_real_kw,
                v->resultado.energia_kwh,
                v->resultado.custo_reais,
                restante < 0.0f ? 0.0f : restante
            );

            if (v->resultado.teve_reducao_potencia)
                fprintf(f, "    <div class='alerta'>⚠ Potência reduzida pelo controle de demanda</div>\n");

        } else if (v->status == VAGA_CONCLUIDA) {
            fprintf(f,
                "    <div class='vaga-veiculo'>%s</div>\n"
                "    <div class='vaga-user'>👤 %s</div>\n"
                "    <div class='vaga-stats'>\n"
                "      <div class='stat'><div class='sl'>Energia</div>"
                "        <div class='sv'>%.2f kWh</div></div>\n"
                "      <div class='stat'><div class='sl'>Custo final</div>"
                "        <div class='sv'>R$ %.2f</div></div>\n"
                "    </div>\n",
                veiculo_esc, user_esc,
                v->resultado.energia_kwh,
                v->resultado.custo_reais
            );
        } else {
            fprintf(f, "    <div style='color:#484f58;font-size:0.9rem;"
                       "margin-top:10px;'>Aguardando veículo...</div>\n");
        }

        fprintf(f, "  </div>\n"); /* fecha .vaga */
    }

    fprintf(f, "</div>\n"); /* fecha .vagas-grid */

    /* ---- Footer ---- */
    fprintf(f,
        "<div class='footer'>\n"
        "  ChargeGrid Intelligence • EV Challenge 2026 • FIAP × GoodWe •"
        "  Esta página atualiza automaticamente a cada 5 segundos\n"
        "</div>\n"
        "</div>\n" /* fecha .container */
        "</body></html>\n"
    );

    fclose(f);
}

/* Escapa caracteres especiais do HTML */
void escapar_html(const char *entrada, char *saida, int max) {
    int i = 0, j = 0;
    while (entrada[i] && j < max - 6) {
        switch (entrada[i]) {
            case '<': strcpy(saida+j, "&lt;");   j+=4; break;
            case '>': strcpy(saida+j, "&gt;");   j+=4; break;
            case '&': strcpy(saida+j, "&amp;");  j+=5; break;
            case '"': strcpy(saida+j, "&quot;"); j+=6; break;
            default:  saida[j++] = entrada[i]; break;
        }
        i++;
    }
    saida[j] = '\0';
}

/* ================================================================
 *  NOVIDADE 2 — PREVISÃO DE FILA
 *
 *  Usa o timestamp_inicio (hora real que o carro conectou)
 *  para calcular quanto tempo ja passou e quanto falta
 *  para cada vaga terminar. Identifica qual vai liberar
 *  primeiro e avisa o próximo usuário.
 * ================================================================ */
float calcular_tempo_restante_min(const Vaga *v) {
    if (v->status != VAGA_OCUPADA) return 0.0f;

    time_t agora    = time(NULL);
    double decorrido = difftime(agora, v->sessao.timestamp_inicio) / 60.0;
    float  restante  = v->resultado.tempo_min - (float)decorrido;
    return restante;
}

void exibir_previsao_fila(const Eletroposto *ep) {
    int i;
    int livres = contar_vagas_livres(ep);

    if (livres > 0) {
        printf("\n  Existem %d vaga(s) livre(s). Sem fila no momento!\n\n", livres);
        return;
    }

    /* Todas ocupadas — encontra qual libera primeiro */
    sep(1);
    printf("  PREVISAO DE FILA — Todas as vagas estao ocupadas\n");
    sep(0);

    int   vaga_mais_rapida    = -1;
    float menor_tempo         = 1e9f;

    printf("\n  Tempo estimado de espera por vaga:\n\n");
    for (i = 0; i < MAX_VAGAS; i++) {
        if (ep->vagas[i].status != VAGA_OCUPADA) continue;

        float restante = calcular_tempo_restante_min(&ep->vagas[i]);
        if (restante < 0.0f) restante = 0.0f;

        printf("    Vaga %d — %s — %.0f min restantes\n",
               ep->vagas[i].numero_vaga,
               ep->vagas[i].sessao.modelo_veiculo,
               restante);

        if (restante < menor_tempo) {
            menor_tempo      = restante;
            vaga_mais_rapida = i;
        }
    }

    if (vaga_mais_rapida >= 0) {
        printf("\n  >> Proximo a liberar: Vaga %d (~%.0f min)\n",
               ep->vagas[vaga_mais_rapida].numero_vaga,
               menor_tempo);
        printf("  >> Veiculo: %s | Usuario: %s\n\n",
               ep->vagas[vaga_mais_rapida].sessao.modelo_veiculo,
               ep->vagas[vaga_mais_rapida].sessao.id_usuario);
    }
    sep(1);
    printf("\n");
}

/* ================================================================
 *  NOVIDADE 3 — NOTIFICAÇÃO WHATSAPP
 *
 *  Usa a API gratuita Callmebot para enviar mensagem no
 *  WhatsApp do usuario quando a recarga termina.
 *  O numero e a chave vem das variaveis de ambiente ou
 *  do telefone cadastrado na sessao.
 *
 *  URL encode converte espacos e acentos para %XX
 *  (exigido pela API do WhatsApp via HTTP GET).
 * ================================================================ */
void url_encode(const char *entrada, char *saida, int max) {
    int i = 0, j = 0;
    while (entrada[i] && j < max - 4) {
        unsigned char c = (unsigned char)entrada[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.') {
            saida[j++] = c;
        } else {
            /* Converte para %HH */
            j += snprintf(saida + j, max - j, "%%%02X", c);
        }
        i++;
    }
    saida[j] = '\0';
}

void notificar_whatsapp(const Sessao *s, const ResultadoSessao *r) {
    /* Pega numero: primeiro da sessao, depois da variavel de ambiente */
    const char *numero = (strlen(s->telefone_whatsapp) > 5)
                         ? s->telefone_whatsapp
                         : getenv("CG_WHATSAPP");
    const char *apikey = getenv("CG_WA_APIKEY");

    if (!numero || !apikey || strlen(numero) < 8 || strlen(apikey) < 4) {
        printf("  [WhatsApp] Nao configurado — pulando notificacao.\n");
        return;
    }

    /* Monta mensagem */
    char mensagem[512];
    snprintf(mensagem, sizeof(mensagem),
        "ChargeGrid: Recarga concluida! "
        "Veiculo: %s. "
        "Energia: %.2f kWh. "
        "Tempo: %.0f min. "
        "Custo: R$ %.2f. "
        "Obrigado por usar o ChargeGrid!",
        s->modelo_veiculo,
        r->energia_kwh,
        r->tempo_min,
        r->custo_reais
    );

    /* URL-encode da mensagem */
    char msg_encoded[1024];
    url_encode(mensagem, msg_encoded, sizeof(msg_encoded));

    /* Monta comando PowerShell para a API Callmebot (compativel Windows) */
    char cmd[MAX_CMD_LEN];
    snprintf(cmd, sizeof(cmd),
        "powershell -Command \"Invoke-WebRequest -Uri "
        "'https://api.callmebot.com/whatsapp.php"
        "?phone=%s&text=%s&apikey=%s' -UseBasicParsing | Out-Null\"",
        numero, msg_encoded, apikey
    );

    printf("\n  [WhatsApp] Enviando notificacao para +%s...\n", numero);
    int ret = system(cmd);
    if (ret == 0)
        printf("  [WhatsApp] Notificacao enviada com sucesso!\n");
    else
        printf("  [WhatsApp] Falha ao enviar (verifique conexao).\n");
}

/* ================================================================
 * ================================================================
 *  FUNCOES DE APOIO — REGRA UNICA, SEM REPETICAO
 *
 *  Antes da refatoracao, cada uma destas regras estava escrita
 *  em dois ou tres lugares diferentes do programa. Se a tarifa
 *  de pico mudasse, era preciso lembrar de alterar em todos eles
 *  — e esquecer um significava calculo inconsistente.
 *  Agora cada regra existe uma unica vez.
 * ================================================================
 * ================================================================ */

/* Tarifa base (R$/kWh) do turno. Antes estava duplicada no switch
   de calcular_sessao() e no if/else de gerar_sessoes_demo(). */
float tarifa_do_turno(TurnoTarifario t) {
    switch (t) {
        case TURNO_OFFPEAK: return TARIFA_OFFPEAK;
        case TURNO_PICO:    return TARIFA_PICO;
        default:            return TARIFA_NORMAL;
    }
}

/* Texto descritivo do turno com a faixa de horario. */
const char *str_turno_detalhado(TurnoTarifario t) {
    switch (t) {
        case TURNO_OFFPEAK: return "Off-peak (00h-06h)";
        case TURNO_PICO:    return "Pico (18h-22h)";
        default:            return "Normal (06h-18h)";
    }
}

/* Desconto de fidelidade conforme o perfil do usuario. */
float desconto_fidelidade(TipoUsuario t) {
    return (t == USUARIO_PREMIUM || t == USUARIO_FROTA)
           ? DESCONTO_FIDELIDADE : 0.0f;
}

/* Energia necessaria para levar a bateria do nivel atual ao alvo.
   Estava repetida em calcular_sessao() e iniciar_sessao_vaga(). */
float energia_necessaria_kwh(const Sessao *s) {
    float delta = s->pct_bateria_alvo - s->pct_bateria_atual;
    return (delta / 100.0f) * s->capacidade_bateria_kwh;
}

/* Tempo de recarga em minutos. Protege contra divisao por zero. */
float tempo_recarga_min(float energia_kwh, float potencia_kw) {
    if (potencia_kw <= 0.0f) return 0.0f;
    return (energia_kwh / potencia_kw) * 60.0f;
}

/* Conta vagas disponiveis para uma nova recarga.
 *
 * CORRECAO: uma vaga em VAGA_CONCLUIDA nao esta mais em uso — o
 * carro ja foi embora e a sessao ja foi gravada no historico. Esse
 * status serve apenas para o painel continuar exibindo o resultado
 * da ultima recarga daquele carregador. Portanto ela conta como
 * disponivel, assim como VAGA_LIVRE.
 *
 * Antes, so VAGA_LIVRE era contada: depois de 5 recargas encerradas
 * a estacao travava permanentemente em "todas as vagas ocupadas". */
int contar_vagas_livres(const Eletroposto *ep) {
    int i, livres = 0;
    for (i = 0; i < MAX_VAGAS; i++)
        if (ep->vagas[i].status != VAGA_OCUPADA) livres++;
    return livres;
}

/* Rotulo visual do status usado no dashboard. Junto com
   str_status_html(), substitui os ternarios que estavam
   escritos direto dentro de gerar_dashboard(). */
const char *str_status_label(StatusVaga s) {
    switch (s) {
        case VAGA_OCUPADA:   return "\u25cf CARREGANDO";
        case VAGA_CONCLUIDA: return "\u2713 CONCLU\u00cdDO";
        default:             return "\u25cb LIVRE";
    }
}

/* Monta o nome de um arquivo temporario unico por processo.
   Havia dois blocos #ifdef identicos fazendo isso. */
void nome_arquivo_temp(const char *prefixo, char *dest, int tam) {
#ifdef _WIN32
    snprintf(dest, tam, "%s_%lu.json", prefixo,
             (unsigned long)GetCurrentProcessId());
#else
    snprintf(dest, tam, "/tmp/%s_%d.json", prefixo, (int)getpid());
#endif
}

/* ================================================================
 * ================================================================
 *  SPRINT 3 — PARTE A: VETOR DE ESTRUTURAS (CADASTRO E LISTAGEM)
 * ================================================================
 * ================================================================ */

/* Zera o historico. Chamada uma unica vez, na inicializacao. */
void inicializar_historico(HistoricoSessoes *h) {
    h->total           = 0;
    h->proximo_id      = 1;
    h->ordenado_por_id = 1;  /* vetor vazio esta trivialmente ordenado */
}

/* ----------------------------------------------------------------
 *  cadastrarSessao — grava uma recarga concluida no vetor.
 *
 *  Complexidade: O(1) — escreve direto na posicao 'total' e
 *  incrementa o contador. Nao precisa percorrer nada.
 * ---------------------------------------------------------------- */
int cadastrar_sessao(HistoricoSessoes *h, const Sessao *s,
                     const ResultadoSessao *r, int numero_vaga) {
    RegistroSessao *reg;
    time_t agora;

    if (h->total >= MAX_SESSOES) {
        printf("  [HISTORICO] Vetor cheio (%d sessoes). Cadastro ignorado.\n",
               MAX_SESSOES);
        return -1;
    }

    reg = &h->itens[h->total];
    memset(reg, 0, sizeof(RegistroSessao));

    reg->id = h->proximo_id++;
    strncpy(reg->id_usuario,     s->id_usuario,     sizeof(reg->id_usuario) - 1);
    strncpy(reg->modelo_veiculo, s->modelo_veiculo, sizeof(reg->modelo_veiculo) - 1);
    reg->tipo_usuario = s->tipo_usuario;
    reg->turno        = s->turno;
    reg->numero_vaga  = numero_vaga;
    reg->hora_inicio  = s->hora_inicio;
    reg->energia_kwh  = r->energia_kwh;
    reg->tempo_min    = r->tempo_min;
    reg->potencia_kw  = s->potencia_real_kw;
    reg->tarifa_final = r->tarifa_final;
    reg->custo_reais  = r->custo_reais;

    agora = time(NULL);
    strftime(reg->data_hora, sizeof(reg->data_hora),
             "%d/%m/%Y %H:%M", localtime(&agora));

    h->total++;
    /* Como os IDs sao gerados em ordem crescente, inserir no fim
       nao quebra a ordenacao por ID caso ela ja existisse. */
    return reg->id;
}

/* ----------------------------------------------------------------
 *  listarSessoes — mostra todo o vetor em formato de tabela.
 *  Complexidade: O(n) — visita cada posicao ocupada uma vez.
 * ---------------------------------------------------------------- */
void listar_sessoes(const HistoricoSessoes *h) {
    int i;

    sep(1);
    printf("  HISTORICO DE SESSOES — %d de %d posicoes ocupadas\n",
           h->total, MAX_SESSOES);
    sep(1);

    if (h->total == 0) {
        printf("\n  Nenhuma sessao registrada ainda.\n");
        printf("  Dica: encerre uma sessao (opcao 2) ou gere dados de\n");
        printf("        demonstracao (opcao 12) para testar busca e ordenacao.\n\n");
        sep(1);
        printf("\n");
        return;
    }

    printf("  %-4s | %-12s | %-20s | %-8s | %10s | %8s | %10s\n",
           "ID", "USUARIO", "VEICULO", "TURNO", "ENERGIA", "TEMPO", "CUSTO");
    sep(0);

    for (i = 0; i < h->total; i++) {
        const RegistroSessao *g = &h->itens[i];
        printf("  %-4d | %-12.12s | %-20.20s | %-8s | %6.2f kWh | %4.0f min | R$ %6.2f\n",
               g->id, g->id_usuario, g->modelo_veiculo, str_turno(g->turno),
               g->energia_kwh, g->tempo_min, g->custo_reais);
    }

    sep(0);
    printf("  Vetor ordenado por ID: %s\n",
           h->ordenado_por_id ? "SIM  (busca binaria disponivel)"
                              : "NAO  (busca binaria indisponivel)");
    sep(1);
    printf("\n");
}

/* Exibe uma unica sessao com todos os campos. */
void exibir_detalhe_sessao(const RegistroSessao *g) {
    sep(0);
    printf("  SESSAO #%d  —  registrada em %s\n", g->id, g->data_hora);
    sep(0);
    printf("  Usuario : %s (%s)\n", g->id_usuario, str_tipo(g->tipo_usuario));
    printf("  Veiculo : %s\n", g->modelo_veiculo);
    printf("  Vaga    : %d   |  Inicio: %02dh  |  Turno: %s\n",
           g->numero_vaga, g->hora_inicio, str_turno(g->turno));
    printf("  Energia : %.2f kWh @ %.1f kW  |  Tempo: %.0f min\n",
           g->energia_kwh, g->potencia_kw, g->tempo_min);
    printf("  Tarifa  : R$ %.2f/kWh  |  CUSTO TOTAL: R$ %.2f\n",
           g->tarifa_final, g->custo_reais);
    sep(0);
}

/* ----------------------------------------------------------------
 *  Gera sessoes de demonstracao.
 *
 *  Serve para o avaliador testar busca, ordenacao e estatisticas
 *  sem precisar digitar dezenas de recargas na mao. Os valores
 *  seguem exatamente as mesmas regras de tarifacao do sistema.
 * ---------------------------------------------------------------- */
void gerar_sessoes_demo(HistoricoSessoes *h) {
    static const char *usuarios[] = {
        "joao01", "maria02", "pedro03", "ana04",  "carlos05", "bruna06",
        "lucas07", "paula08", "rafael09", "ines10", "tiago11", "vera12"
    };
    static const char *veiculos[] = {
        "BYD Dolphin", "Fiat Fastback HEV", "Tesla Model 3", "Renault Kwid E-Tech",
        "Volvo EX30", "GWM Ora 03", "Nissan Leaf", "BMW iX1",
        "Caoa Chery iCar", "Peugeot e-208", "Volvo XC40 Recharge", "JAC E-JS1"
    };
    static const float energias[]  = { 36.00f, 42.75f, 57.30f,  8.70f, 24.50f, 31.20f,
                                       15.40f, 48.90f, 12.60f, 39.15f, 52.00f, 20.30f };
    static const float potencias[] = { 22.0f, 11.0f, 22.0f,  7.4f, 11.0f, 22.0f,
                                        7.4f, 22.0f,  3.7f, 11.0f, 22.0f, 11.0f };
    static const int   horas[]     = { 19, 14,  3,  9, 20, 11,  2, 18,  7, 22,  4, 16 };
    static const int   tipos[]     = {  2,  1,  3,  1,  2,  1,  3,  2,  1,  1,  3,  2 };

    const int quantidade = 12;
    int i, criadas = 0;
    time_t agora = time(NULL);

    for (i = 0; i < quantidade && h->total < MAX_SESSOES; i++) {
        RegistroSessao *g = &h->itens[h->total];

        memset(g, 0, sizeof(RegistroSessao));

        g->id = h->proximo_id++;
        strncpy(g->id_usuario,     usuarios[i], sizeof(g->id_usuario) - 1);
        strncpy(g->modelo_veiculo, veiculos[i], sizeof(g->modelo_veiculo) - 1);
        g->tipo_usuario = (TipoUsuario)tipos[i];
        g->hora_inicio  = horas[i];
        g->turno        = definir_turno(horas[i]);
        g->numero_vaga  = (i % MAX_VAGAS) + 1;
        g->energia_kwh  = energias[i];
        g->potencia_kw  = potencias[i];
        g->tempo_min    = tempo_recarga_min(energias[i], potencias[i]);

        /* Usa exatamente as mesmas regras de calcular_sessao() */
        g->tarifa_final = tarifa_do_turno(g->turno)
                          * (1.0f - desconto_fidelidade(g->tipo_usuario));
        g->custo_reais  = g->energia_kwh * g->tarifa_final;

        strftime(g->data_hora, sizeof(g->data_hora),
                 "%d/%m/%Y %H:%M", localtime(&agora));

        h->total++;
        criadas++;
    }

    printf("\n  %d sessoes de demonstracao adicionadas ao historico.\n", criadas);
    printf("  Total no vetor: %d de %d posicoes.\n", h->total, MAX_SESSOES);
    printf("  Use a opcao 8 para listar, 9 para buscar e 10 para ordenar.\n\n");
}

/* ================================================================
 * ================================================================
 *  SPRINT 3 — PARTE B: ALGORITMOS DE BUSCA
 * ================================================================
 * ================================================================ */

/* ----------------------------------------------------------------
 *  BUSCA LINEAR — percorre o vetor do inicio ao fim comparando
 *  cada ID com o procurado. Para assim que encontra.
 *
 *  Melhor caso : O(1)   — alvo na primeira posicao
 *  Pior caso   : O(n)   — alvo na ultima posicao ou inexistente
 *  Caso medio  : O(n)   — em media n/2 comparacoes
 *
 *  Nao exige vetor ordenado.
 *  Retorna o INDICE no vetor, ou -1 se nao encontrar.
 * ---------------------------------------------------------------- */
int busca_linear_id(const HistoricoSessoes *h, int id, long *comparacoes) {
    int i;
    *comparacoes = 0;

    for (i = 0; i < h->total; i++) {
        (*comparacoes)++;                 /* 1 comparacao por posicao */
        if (h->itens[i].id == id) return i;
    }
    return -1;
}

/* ----------------------------------------------------------------
 *  BUSCA BINARIA — divide o intervalo de busca pela metade a cada
 *  passo, descartando sempre metade dos elementos restantes.
 *
 *  Melhor caso : O(1)       — alvo bem no meio
 *  Pior caso   : O(log n)   — n=100 exige no maximo 7 comparacoes
 *  PRE-REQUISITO: o vetor PRECISA estar ordenado pela chave (ID).
 *
 *  Retorna o INDICE no vetor, ou -1 se nao encontrar.
 * ---------------------------------------------------------------- */
int busca_binaria_id(const HistoricoSessoes *h, int id, long *comparacoes) {
    int inicio = 0;
    int fim    = h->total - 1;
    int meio;

    *comparacoes = 0;

    while (inicio <= fim) {
        meio = inicio + (fim - inicio) / 2;   /* evita estouro de int */
        (*comparacoes)++;

        if (h->itens[meio].id == id)  return meio;      /* achou      */
        if (h->itens[meio].id <  id)  inicio = meio + 1; /* olha a direita */
        else                          fim    = meio - 1; /* olha a esquerda */
    }
    return -1;
}

/* ----------------------------------------------------------------
 *  BUSCA LINEAR POR USUARIO — varredura completa que lista TODAS
 *  as sessoes cujo ID de usuario contem o texto digitado.
 *
 *  Complexidade: O(n * m), onde n = numero de sessoes e
 *  m = tamanho do termo buscado (custo da comparacao de texto).
 *  Nao para no primeiro achado, pois queremos todos os resultados.
 * ---------------------------------------------------------------- */
void busca_linear_usuario(const HistoricoSessoes *h, const char *termo) {
    int i, achados = 0;
    long comparacoes = 0;

    sep(0);
    printf("  Resultados para \"%s\":\n", termo);
    sep(0);

    for (i = 0; i < h->total; i++) {
        comparacoes++;
        if (contem_texto(h->itens[i].id_usuario, termo) ||
            contem_texto(h->itens[i].modelo_veiculo, termo)) {
            const RegistroSessao *g = &h->itens[i];
            printf("  #%-3d | %-12.12s | %-20.20s | %6.2f kWh | R$ %6.2f\n",
                   g->id, g->id_usuario, g->modelo_veiculo,
                   g->energia_kwh, g->custo_reais);
            achados++;
        }
    }

    sep(0);
    if (achados == 0) printf("  Nenhuma sessao encontrada.\n");
    else              printf("  %d sessao(oes) encontrada(s).\n", achados);
    /* CORRECAO: esta busca compara TEXTO, entao o custo e O(n*m):
       n sessoes x m = custo de comparar as strings. O rotulo antigo
       dizia O(n), divergindo da documentacao da propria funcao. */
    printf("  Comparacoes realizadas: %ld registros x custo textual  (O(n*m))\n",
           comparacoes);
    sep(0);
    printf("\n");
}

/* ----------------------------------------------------------------
 *  MENU DE BUSCA
 * ---------------------------------------------------------------- */
void menu_buscar_sessao(HistoricoSessoes *h) {
    int opcao, id, pos = -1;   /* pos inicializado por seguranca */
    long comparacoes = 0;

    if (h->total == 0) {
        printf("\n  Nenhuma sessao registrada ainda.\n");
        printf("  Use a opcao 12 para gerar sessoes de demonstracao.\n\n");
        return;
    }

    sep(1);
    printf("  BUSCAR SESSAO — %d registros no vetor\n", h->total);
    sep(0);
    printf("  1. Busca LINEAR por ID          [ O(n)     ]\n");
    printf("  2. Busca BINARIA por ID         [ O(log n) ]\n");
    printf("  3. Busca por usuario ou veiculo [ O(n)     ]\n");
    printf("  4. Comparar linear x binaria    [ demonstracao ]\n");
    printf("  0. Voltar\n");
    sep(0);

    opcao = ler_int("  Opcao > ", 0, 4);
    if (opcao == 0) { printf("\n"); return; }

    if (opcao == 3) {
        char termo[50];
        ler_texto("\n  Digite parte do usuario ou do veiculo: ", termo, sizeof(termo));
        if (strlen(termo) == 0) { printf("  Termo vazio.\n\n"); return; }
        busca_linear_usuario(h, termo);
        return;
    }

    /* Opcoes 2 e 4 exigem o vetor ordenado por ID */
    if ((opcao == 2 || opcao == 4) && !h->ordenado_por_id) {
        long c, t;
        printf("\n  [AVISO] A busca binaria exige o vetor ordenado por ID,\n");
        printf("          e ele esta ordenado por outro criterio.\n");
        printf("          Reordenando por ID com Insertion Sort...\n");
        insertion_sort_sessoes(h, ORD_ID, 0, &c, &t);
        printf("          Pronto: %ld comparacoes, %ld movimentacoes.\n", c, t);
    }

    id = ler_int("\n  Digite o ID da sessao: ", 1, 9999);

    if (opcao == 1 || opcao == 4) {
        pos = busca_linear_id(h, id, &comparacoes);
        printf("\n  >> BUSCA LINEAR   : %ld comparacao(oes)  |  %s\n",
               comparacoes, pos >= 0 ? "ENCONTRADA" : "nao encontrada");
    }

    if (opcao == 2 || opcao == 4) {
        long comp_bin = 0;
        pos = busca_binaria_id(h, id, &comp_bin);
        printf("  >> BUSCA BINARIA  : %ld comparacao(oes)  |  %s\n",
               comp_bin, pos >= 0 ? "ENCONTRADA" : "nao encontrada");
        if (opcao == 4)
            printf("\n  Com n = %d registros, a binaria precisou de bem menos\n"
                   "  comparacoes porque descarta metade do vetor a cada passo.\n",
                   h->total);
        comparacoes = comp_bin;
    }

    printf("\n");
    if (pos >= 0) exibir_detalhe_sessao(&h->itens[pos]);
    else          printf("  Nenhuma sessao com ID %d foi encontrada.\n", id);
    printf("\n");
}

/* ================================================================
 * ================================================================
 *  SPRINT 3 — PARTE C: ALGORITMOS DE ORDENACAO
 *  (implementados manualmente, sem usar qsort da biblioteca)
 * ================================================================
 * ================================================================ */

/* ----------------------------------------------------------------
 *  Funcao de comparacao usada pelos dois algoritmos.
 *  Retorna:  > 0 se 'a' deve vir DEPOIS de 'b'
 *            < 0 se 'a' deve vir ANTES de 'b'
 *            = 0 se sao equivalentes no criterio escolhido
 *  Complexidade: O(1)
 * ---------------------------------------------------------------- */
int comparar_registros(const RegistroSessao *a, const RegistroSessao *b,
                       CriterioOrdenacao crit, int decrescente) {
    float va, vb;
    int   resultado;

    /* CORRECAO: o ID e uma chave INTEIRA e deve ser comparado como
       int. Converter para float era um erro de tipo: a mantissa de
       um float tem 24 bits, entao a partir de 16.777.217 IDs
       distintos passariam a ser vistos como iguais. Comparacao
       entre inteiros e sempre exata. */
    if (crit == ORD_ID) {
        if      (a->id > b->id) resultado =  1;
        else if (a->id < b->id) resultado = -1;
        else                    resultado =  0;
        return decrescente ? -resultado : resultado;
    }

    switch (crit) {
        case ORD_ENERGIA: va = a->energia_kwh; vb = b->energia_kwh; break;
        case ORD_CUSTO:   va = a->custo_reais; vb = b->custo_reais; break;
        default:          va = a->tempo_min;   vb = b->tempo_min;   break;
    }

    if (va > vb)      resultado =  1;
    else if (va < vb) resultado = -1;
    else              resultado =  0;

    return decrescente ? -resultado : resultado;
}

/* ----------------------------------------------------------------
 *  BUBBLE SORT
 *
 *  Compara pares vizinhos e troca quando estao fora de ordem.
 *  A cada passada completa, o maior elemento "borbulha" para o
 *  fim do vetor — por isso o laco interno anda uma posicao a
 *  menos a cada volta (h->total - 1 - i).
 *
 *  Melhor caso : O(n)    — vetor ja ordenado, sai na 1a passada
 *                          gracas a flag 'houve_troca'
 *  Pior caso   : O(n^2)  — vetor em ordem inversa
 *  Caso medio  : O(n^2)
 *  Espaco      : O(1)    — ordena dentro do proprio vetor
 *
 *  De onde vem o n^2: sao dois lacos aninhados. O externo roda
 *  (n-1) vezes e o interno roda em media n/2 vezes, resultando
 *  em aproximadamente n(n-1)/2 comparacoes.
 * ---------------------------------------------------------------- */
void bubble_sort_sessoes(HistoricoSessoes *h, CriterioOrdenacao crit,
                         int decrescente, long *comparacoes, long *trocas) {
    int i, j, houve_troca;

    *comparacoes = 0;
    *trocas      = 0;

    for (i = 0; i < h->total - 1; i++) {          /* laco externo: n-1 passadas */
        houve_troca = 0;

        for (j = 0; j < h->total - 1 - i; j++) {  /* laco interno: vizinhos     */
            (*comparacoes)++;
            if (comparar_registros(&h->itens[j], &h->itens[j + 1],
                                   crit, decrescente) > 0) {
                RegistroSessao temp = h->itens[j];
                h->itens[j]     = h->itens[j + 1];
                h->itens[j + 1] = temp;
                (*trocas)++;
                houve_troca = 1;
            }
        }

        /* Otimizacao: se nenhuma troca ocorreu, o vetor ja esta
           ordenado e podemos parar. E isso que da o melhor caso O(n). */
        if (!houve_troca) break;
    }

    h->ordenado_por_id = (crit == ORD_ID && !decrescente);
}

/* ----------------------------------------------------------------
 *  INSERTION SORT
 *
 *  Percorre o vetor da esquerda para a direita tratando a parte
 *  ja visitada como "ordenada". Cada novo elemento (a chave) e
 *  deslocado para tras ate encontrar sua posicao correta.
 *
 *  Melhor caso : O(n)    — vetor ja ordenado, o while nunca entra
 *  Pior caso   : O(n^2)  — vetor em ordem inversa, cada chave
 *                          precisa voltar ate o inicio
 *  Espaco      : O(1)
 *
 *  Na pratica costuma fazer bem menos movimentacoes que o Bubble
 *  Sort, mesmo tendo a mesma classe de complexidade.
 * ---------------------------------------------------------------- */
void insertion_sort_sessoes(HistoricoSessoes *h, CriterioOrdenacao crit,
                            int decrescente, long *comparacoes, long *trocas) {
    int i, j;

    *comparacoes = 0;
    *trocas      = 0;

    for (i = 1; i < h->total; i++) {
        RegistroSessao chave = h->itens[i];   /* elemento a posicionar */
        j = i - 1;

        /* Empurra para a direita todo mundo que for "maior" que a chave */
        while (j >= 0) {
            (*comparacoes)++;
            if (comparar_registros(&h->itens[j], &chave, crit, decrescente) <= 0)
                break;                        /* achou o lugar da chave */
            h->itens[j + 1] = h->itens[j];
            (*trocas)++;
            j--;
        }
        h->itens[j + 1] = chave;
    }

    h->ordenado_por_id = (crit == ORD_ID && !decrescente);
}

const char *str_criterio(CriterioOrdenacao c) {
    switch (c) {
        case ORD_ENERGIA: return "energia (kWh)";
        case ORD_CUSTO:   return "custo (R$)";
        case ORD_TEMPO:   return "tempo (min)";
        default:          return "ID";
    }
}

/* ----------------------------------------------------------------
 *  MENU DE ORDENACAO
 * ---------------------------------------------------------------- */
void menu_ordenar_sessoes(HistoricoSessoes *h) {
    int algoritmo, criterio, ordem;
    long comparacoes = 0, trocas = 0;
    const char *nome_algoritmo;

    if (h->total < 2) {
        printf("\n  E preciso ter pelo menos 2 sessoes para ordenar.\n");
        printf("  Use a opcao 12 para gerar sessoes de demonstracao.\n\n");
        return;
    }

    sep(1);
    printf("  ORDENAR SESSOES — %d registros no vetor\n", h->total);
    sep(0);
    printf("  Algoritmo:\n");
    printf("    1. Bubble Sort     [ O(n^2) — O(n) no melhor caso ]\n");
    printf("    2. Insertion Sort  [ O(n^2) — O(n) no melhor caso ]\n");
    sep(0);
    algoritmo = ler_int("  Algoritmo > ", 1, 2);

    printf("\n  Criterio:\n");
    printf("    1. ID da sessao\n");
    printf("    2. Energia consumida (kWh)\n");
    printf("    3. Custo da sessao (R$)\n");
    printf("    4. Tempo de recarga (min)\n");
    criterio = ler_int("  Criterio > ", 1, 4);

    printf("\n  Ordem:\n");
    printf("    1. Crescente (menor para o maior)\n");
    printf("    2. Decrescente (maior para o menor)\n");
    ordem = ler_int("  Ordem > ", 1, 2);

    if (algoritmo == 1) {
        nome_algoritmo = "Bubble Sort";
        bubble_sort_sessoes(h, (CriterioOrdenacao)criterio,
                            ordem == 2, &comparacoes, &trocas);
    } else {
        nome_algoritmo = "Insertion Sort";
        insertion_sort_sessoes(h, (CriterioOrdenacao)criterio,
                               ordem == 2, &comparacoes, &trocas);
    }

    printf("\n");
    sep(1);
    printf("  ORDENACAO CONCLUIDA\n");
    sep(0);
    printf("  Algoritmo    : %s\n", nome_algoritmo);
    printf("  Criterio     : %s (%s)\n",
           str_criterio((CriterioOrdenacao)criterio),
           ordem == 2 ? "decrescente" : "crescente");
    printf("  Elementos    : n = %d\n", h->total);
    printf("  Comparacoes  : %ld\n", comparacoes);
    printf("  Movimentacoes: %ld\n", trocas);
    printf("  Referencia   : n^2 = %d  |  n*log2(n) = %.0f\n",
           h->total * h->total,
           h->total * (log((double)h->total) / log(2.0)));
    sep(1);

    listar_sessoes(h);
}

/* ================================================================
 * ================================================================
 *  SPRINT 3 — PARTE D: ESTATISTICAS DA ESTACAO
 * ================================================================
 * ================================================================ */

/* ----------------------------------------------------------------
 *  calcular_resumo_historico — UMA varredura O(n) que produz todos
 *  os indicadores de uma vez. Tanto mostrar_estatisticas() quanto
 *  gerar_dashboard() consomem este resultado, em vez de cada um
 *  percorrer o vetor com o mesmo calculo repetido.
 *
 *  Complexidade: O(n) — um unico laco, sem lacos aninhados.
 * ---------------------------------------------------------------- */
void calcular_resumo_historico(const HistoricoSessoes *h, ResumoHistorico *res) {
    int i;

    memset(res, 0, sizeof(ResumoHistorico));
    res->total_sessoes = h->total;
    if (h->total == 0) return;

    res->maior_consumo = h->itens[0].energia_kwh;
    res->menor_consumo = h->itens[0].energia_kwh;

    for (i = 0; i < h->total; i++) {
        const RegistroSessao *g = &h->itens[i];

        res->energia_total += g->energia_kwh;
        res->faturamento   += g->custo_reais;
        res->tempo_total   += g->tempo_min;

        if (g->energia_kwh > res->maior_consumo) {
            res->maior_consumo = g->energia_kwh;
            res->indice_maior  = i;
        }
        if (g->energia_kwh < res->menor_consumo) {
            res->menor_consumo = g->energia_kwh;
            res->indice_menor  = i;
        }

        if (g->turno >= 1 && g->turno <= 3)               res->por_turno[g->turno]++;
        if (g->tipo_usuario >= 1 && g->tipo_usuario <= 3) res->por_tipo[g->tipo_usuario]++;
    }
}

/* ----------------------------------------------------------------
 *  mostrarEstatisticas — apenas formata e exibe o resumo.
 *  Todo o calculo fica em calcular_resumo_historico().
 * ---------------------------------------------------------------- */
void mostrar_estatisticas(const HistoricoSessoes *h) {
    ResumoHistorico res;
    int j;

    sep(1);
    printf("  ESTATISTICAS DA ESTACAO\n");
    sep(1);

    if (h->total == 0) {
        printf("\n  Nenhuma sessao registrada ainda.\n");
        printf("  Use a opcao 12 para gerar sessoes de demonstracao.\n\n");
        sep(1);
        printf("\n");
        return;
    }

    calcular_resumo_historico(h, &res);

    printf("\n  Sessoes realizadas : %d\n", res.total_sessoes);
    printf("  Energia fornecida  : %.2f kWh\n", res.energia_total);
    printf("  Faturamento total  : R$ %.2f\n", res.faturamento);
    printf("  Ticket medio       : R$ %.2f\n", res.faturamento / res.total_sessoes);
    printf("  Consumo medio      : %.2f kWh por sessao\n",
           res.energia_total / res.total_sessoes);
    printf("  Tempo medio        : %.0f min por sessao\n",
           res.tempo_total / res.total_sessoes);
    /* CORRECAO: energia_total pode ser 0 se todas as sessoes tiverem
       consumo nulo. Divisao por zero em float gera "inf" ou "nan"
       impresso no relatorio. */
    printf("  Tarifa media       : R$ %.2f/kWh\n",
           res.energia_total > 0.0f ? res.faturamento / res.energia_total : 0.0f);

    sep(0);
    printf("  Maior consumo : %6.2f kWh  —  sessao #%d (%s, %s)\n",
           res.maior_consumo, h->itens[res.indice_maior].id,
           h->itens[res.indice_maior].id_usuario,
           h->itens[res.indice_maior].modelo_veiculo);
    printf("  Menor consumo : %6.2f kWh  —  sessao #%d (%s, %s)\n",
           res.menor_consumo, h->itens[res.indice_menor].id,
           h->itens[res.indice_menor].id_usuario,
           h->itens[res.indice_menor].modelo_veiculo);

    sep(0);
    printf("  Distribuicao por turno tarifario:\n\n");
    for (j = 1; j <= 3; j++) {
        int barras = (res.por_turno[j] * 30) / res.total_sessoes;
        int k;
        printf("    %-9s [", str_turno((TurnoTarifario)j));
        for (k = 0; k < 30; k++) printf(k < barras ? "#" : ".");
        printf("] %2d sessao(oes)\n", res.por_turno[j]);
    }

    printf("\n  Distribuicao por tipo de usuario:\n");
    printf("    Comum: %d   |   Premium: %d   |   Frota: %d\n",
           res.por_tipo[1], res.por_tipo[2], res.por_tipo[3]);

    sep(0);
    printf("  Impacto ambiental estimado:\n");
    printf("    CO2 evitado: %.1f kg  (vs. veiculo a combustao)\n",
           res.energia_total * 0.233f);
    sep(1);
    printf("\n");
}

/* ================================================================
 *  SPRINT 3 — FUNCOES AUXILIARES
 * ================================================================ */

/* Le uma linha de texto do teclado, tratando o '\n' que sobra
   do scanf usado nas outras leituras. */
void ler_texto(const char *msg, char *dest, int tam) {
    int c;
    printf("%s", msg);

    /* Descarta o restante da linha pendente do scanf anterior,
       para que a leitura comece limpa na proxima linha digitada. */
    while ((c = getchar()) != '\n' && c != EOF) { }

    if (fgets(dest, tam, stdin)) {
        int len = (int)strlen(dest);
        if (len > 0 && dest[len - 1] == '\n') dest[len - 1] = '\0';
    } else {
        dest[0] = '\0';
    }
}

/* Verifica se 'busca' aparece dentro de 'texto', ignorando
   maiusculas e minusculas. Implementado manualmente para nao
   depender de funcoes nao padronizadas como strcasestr. */
int contem_texto(const char *texto, const char *busca) {
    int i, j;
    if (!busca[0]) return 1;

    for (i = 0; texto[i]; i++) {
        for (j = 0; busca[j]; j++) {
            char a = texto[i + j];
            char b = busca[j];
            if (a >= 'A' && a <= 'Z') a = (char)(a + 32);
            if (b >= 'A' && b <= 'Z') b = (char)(b + 32);
            if (a != b) break;
        }
        if (!busca[j]) return 1;   /* percorreu 'busca' inteiro: casou */
    }
    return 0;
}

/* ================================================================
 *  INICIALIZAÇÃO
 * ================================================================ */
void inicializar_eletroposto(Eletroposto *ep) {
    int i;
    for (i = 0; i < MAX_VAGAS; i++) {
        ep->vagas[i].numero_vaga = i + 1;
        ep->vagas[i].status      = VAGA_LIVRE;
        memset(&ep->vagas[i].sessao,    0, sizeof(Sessao));
        memset(&ep->vagas[i].resultado, 0, sizeof(ResultadoSessao));
    }
    ep->log.total            = 0;
    ep->total_sessoes_ativas = 0;
    ep->demanda_atual_kw     = 0.0f;
    ep->hora_sistema         = 10;

    inicializar_historico(&ep->historico);   /* NOVO Sprint 3 */
}

/* ================================================================
 *  MENU PRINCIPAL — agora com opções 8 e 9
 * ================================================================ */
void menu_principal(Eletroposto *ep) {
    int opcao, sair = 0;

    while (!sair) {
        sep(1);
        printf("\n  CHARGEGRID — MENU PRINCIPAL\n");
        int livres = contar_vagas_livres(ep);
        printf("  %02dh  |  Demanda: %.1f/%.0f kW  |  Livres: %d/%d  |  dashboard.html\n\n",
               ep->hora_sistema, ep->demanda_atual_kw,
               DEMANDA_MAX_TOTAL_KW, livres, MAX_VAGAS);
        printf("  Historico: %d sessao(oes) registrada(s) no vetor\n",
               ep->historico.total);
        sep(0);
        printf("  --- OPERACAO DO ELETROPOSTO ---\n");
        printf("   1. Iniciar nova sessao de recarga\n");
        printf("   2. Encerrar sessao ativa\n");
        printf("   3. Painel de vagas\n");
        printf("   4. Relatorio das vagas atuais\n");
        printf("   5. Previsao de fila\n");
        printf("   6. Log OCPP 1.6\n");
        printf("   7. Atualizar dashboard.html\n\n");
        printf("  --- HISTORICO E ALGORITMOS ---\n");
        printf("   8. Listar sessoes registradas\n");
        printf("   9. Buscar sessao        [linear / binaria]\n");
        printf("  10. Ordenar sessoes      [bubble / insertion]\n");
        printf("  11. Estatisticas da estacao\n");
        printf("  12. Gerar sessoes de demonstracao\n\n");
        printf("  --- EXTRAS ---\n");
        printf("  13. Chatbot IA\n");
        printf("  14. Alterar hora do sistema\n");
        printf("   0. Encerrar\n\n");
        sep(0);

        opcao = ler_int("  Opcao > ", 0, 14);

        switch (opcao) {
            /* Operacao do eletroposto */
            case 1:  menu_sessao_nova(ep);              break;
            case 2:  menu_encerrar_sessao(ep);          break;
            case 3:  exibir_painel_vagas(ep);           break;
            case 4:  exibir_relatorio_consolidado(ep);  break;
            case 5:  exibir_previsao_fila(ep);          break;
            case 6:  exibir_log_ocpp(&ep->log);         break;
            case 7:
                gerar_dashboard(ep);
                printf("  Dashboard atualizado! Abra dashboard.html no navegador.\n\n");
                break;

            /* Sprint 3 — estruturas de dados e algoritmos */
            case 8:  listar_sessoes(&ep->historico);    break;
            case 9:  menu_buscar_sessao(&ep->historico); break;
            case 10: menu_ordenar_sessoes(&ep->historico); break;
            case 11: mostrar_estatisticas(&ep->historico); break;
            case 12:
                gerar_sessoes_demo(&ep->historico);
                gerar_dashboard(ep);
                break;

            /* Extras */
            case 13: menu_chatbot(ep);                  break;
            case 14:
                ep->hora_sistema = ler_int("  Nova hora [0-23]: ", 0, 23);
                printf("  Hora: %02dh\n", ep->hora_sistema);
                gerar_dashboard(ep);
                break;

            case 0: sair = 1; break;
        }
    }
}

/* ================================================================
 *  SESSÃO NOVA — agora pede telefone opcional
 * ================================================================ */
void menu_sessao_nova(Eletroposto *ep) {
    int idx = encontrar_vaga_livre(ep);
    if (idx < 0) {
        printf("\n  Todas as vagas estao ocupadas!\n");
        exibir_previsao_fila(ep); /* mostra previsao automaticamente */
        return;
    }
    printf("\n  Vaga %d disponivel.\n", idx + 1);
    iniciar_sessao_vaga(ep, idx);
}

void menu_encerrar_sessao(Eletroposto *ep) {
    int tem = 0, i;
    printf("\n  Vagas ativas:\n");
    for (i = 0; i < MAX_VAGAS; i++) {
        if (ep->vagas[i].status == VAGA_OCUPADA) {
            printf("    Vaga %d — %s (%s)\n",
                   ep->vagas[i].numero_vaga,
                   ep->vagas[i].sessao.modelo_veiculo,
                   ep->vagas[i].sessao.id_usuario);
            tem = 1;
        }
    }
    if (!tem) { printf("    Nenhuma sessao ativa.\n\n"); return; }

    int num = ler_int("\n  Numero da vaga [1-5]: ", 1, MAX_VAGAS);
    int idx = num - 1;
    if (ep->vagas[idx].status != VAGA_OCUPADA) {
        printf("  Vaga %d nao esta ocupada.\n\n", num); return;
    }
    encerrar_sessao_vaga(ep, idx);
}

void menu_chatbot(Eletroposto *ep) {
    int i, escolha = -1;
    for (i = 0; i < MAX_VAGAS; i++)
        if (ep->vagas[i].status == VAGA_OCUPADA) { escolha = i; break; }
    if (escolha < 0)
        for (i = 0; i < MAX_VAGAS; i++)
            if (ep->vagas[i].status == VAGA_CONCLUIDA) { escolha = i; break; }
    if (escolha < 0) {
        printf("\n  Nenhuma sessao registrada ainda.\n\n"); return;
    }
    loop_chatbot(&ep->vagas[escolha].sessao, &ep->vagas[escolha].resultado);
}

/* ================================================================
 *  CONTROLE DE DEMANDA
 * ================================================================ */
float controle_demanda(float pot, const Eletroposto *ep) {
    float atual = calcular_demanda_total(ep);
    float disp  = DEMANDA_MAX_TOTAL_KW - atual;
    if (disp <= 0.0f) {
        printf("\n  [CONTROLE] Rede no limite. Potencia minima: %.1f kW\n", POTENCIA_MIN_KW);
        return POTENCIA_MIN_KW;
    }
    if (pot > disp) {
        float adj = disp < POTENCIA_MIN_KW ? POTENCIA_MIN_KW : disp;
        printf("\n  [CONTROLE] Potencia reduzida de %.1f para %.1f kW\n", pot, adj);
        return adj;
    }
    return pot;
}

float calcular_demanda_total(const Eletroposto *ep) {
    float t = 0.0f; int i;
    for (i = 0; i < MAX_VAGAS; i++)
        if (ep->vagas[i].status == VAGA_OCUPADA)
            t += ep->vagas[i].sessao.potencia_real_kw;
    return t;
}

/* ================================================================
 *  INICIAR SESSÃO — agora salva timestamp e regenera dashboard
 * ================================================================ */
void iniciar_sessao_vaga(Eletroposto *ep, int idx) {
    Vaga *v = &ep->vagas[idx];
    sep(1);
    printf("\n  NOVA SESSAO — VAGA %d\n", v->numero_vaga);
    sep(0);

    v->sessao = coletar_dados_sessao(ep);
    v->sessao.timestamp_inicio = time(NULL); /* NOVO: salva hora real */

    float pot = controle_demanda(v->sessao.potencia_kw, ep);
    v->sessao.potencia_real_kw = pot;
    v->resultado = calcular_sessao(&v->sessao, ep);

    if (pot < v->sessao.potencia_kw) {
        v->resultado.teve_reducao_potencia = 1;
        /* Recalcula com a potencia realmente aplicada, usando as
           mesmas funcoes de calcular_sessao() — sem repetir a formula */
        v->resultado.energia_kwh = energia_necessaria_kwh(&v->sessao);
        v->resultado.tempo_min   = tempo_recarga_min(v->resultado.energia_kwh, pot);
    }

    v->status = VAGA_OCUPADA;
    ep->total_sessoes_ativas++;
    ep->demanda_atual_kw = calcular_demanda_total(ep);

    simular_progresso(&v->sessao, &v->resultado);
    exibir_relatorio(&v->sessao, &v->resultado);
    ocpp_status_notification(ep, idx, VAGA_OCUPADA);
    ocpp_start_transaction(ep, idx, &v->sessao);

    gerar_dashboard(ep); /* NOVO: atualiza dashboard */
    printf("\n  Sessao na vaga %d iniciada. Dashboard atualizado!\n\n", v->numero_vaga);
}

/* ================================================================
 *  ENCERRAR SESSÃO — agora envia WhatsApp e regenera dashboard
 * ================================================================ */
void encerrar_sessao_vaga(Eletroposto *ep, int idx) {
    Vaga *v = &ep->vagas[idx];
    int   id_registro;

    ocpp_meter_values(ep, idx, v->resultado.energia_kwh, v->sessao.pct_bateria_alvo);
    ocpp_stop_transaction(ep, idx, &v->resultado);
    ocpp_status_notification(ep, idx, VAGA_LIVRE);

    v->status = VAGA_CONCLUIDA;
    ep->total_sessoes_ativas--;
    ep->demanda_atual_kw = calcular_demanda_total(ep);

    sep(1);
    printf("  SESSAO ENCERRADA — VAGA %d\n", v->numero_vaga);
    sep(0);
    printf("  Veiculo : %s | Usuario: %s\n",
           v->sessao.modelo_veiculo, v->sessao.id_usuario);
    printf("  Energia : %.2f kWh  |  Tempo: %.0f min\n",
           v->resultado.energia_kwh, v->resultado.tempo_min);
    printf("  CUSTO FINAL: R$ %.2f\n", v->resultado.custo_reais);
    sep(1);

    /* NOVO Sprint 3 — grava a sessao no vetor de estruturas */
    id_registro = cadastrar_sessao(&ep->historico, &v->sessao,
                                   &v->resultado, v->numero_vaga);
    if (id_registro > 0)
        printf("  [HISTORICO] Sessao registrada com ID #%d "
               "(%d de %d posicoes ocupadas).\n",
               id_registro, ep->historico.total, MAX_SESSOES);

    notificar_whatsapp(&v->sessao, &v->resultado); /* NOVO */
    gerar_dashboard(ep);                           /* NOVO */
    printf("\n  Dashboard atualizado!\n\n");
}

/* ================================================================
 *  COLETA DADOS — agora pede telefone (opcional)
 * ================================================================ */
Sessao coletar_dados_sessao(const Eletroposto *ep) {
    Sessao s;
    memset(&s, 0, sizeof(s));

    printf("\n  ID do usuario: ");
    scanf("%19s", s.id_usuario);

    s.tipo_usuario = menu_tipo_usuario();

    printf("\n  Modelo do veiculo: ");
    scanf(" %49[^\n]", s.modelo_veiculo);

    /* Telefone para WhatsApp (opcional) */
    printf("  Telefone WhatsApp (com DDI+DDD, ex: 5511999999999)\n");
    printf("  [Enter para pular]: ");
    {
        int  c;
        char linha[30];
        /* Descarta o restante da linha deixada pelo scanf anterior.
           Sem isso, um Enter sozinho nao pulava o campo: o programa
           acabava lendo o proximo valor digitado como telefone. */
        while ((c = getchar()) != '\n' && c != EOF) { }

        if (fgets(linha, sizeof(linha), stdin)) {
            int len = (int)strlen(linha);
            if (len > 0 && linha[len-1] == '\n') linha[len-1] = '\0';
            if (strlen(linha) > 4)
                strncpy(s.telefone_whatsapp, linha, 19);
        }
    }

    s.capacidade_bateria_kwh = ler_float("\n  Capacidade da bateria (kWh) [10-100]: ", 10.0f, 100.0f);
    s.pct_bateria_atual      = ler_float("  Nivel atual (%%) [0-99]: ", 0.0f, 99.0f);

    float alvo_min = s.pct_bateria_atual + 1.0f;
    char  msg[80];
    sprintf(msg, "  Nivel alvo (%%) [%.0f-100]: ", alvo_min);
    s.pct_bateria_alvo = ler_float(msg, alvo_min, 100.0f);

    s.potencia_kw     = ler_float("  Potencia (kW) [3.7-22.0]: ", POTENCIA_MIN_KW, POTENCIA_MAX_KW);
    s.hora_inicio     = ep->hora_sistema;
    s.turno           = definir_turno(s.hora_inicio);
    s.minutos_reserva = ler_int("  Reserva antecipada (min) [0=nao]: ", 0, 300);
    s.potencia_real_kw = s.potencia_kw;

    printf("  Hora: %02dh — Turno: %s\n", s.hora_inicio, str_turno(s.turno));
    return s;
}

/* ================================================================
 *  CALCULA SESSÃO (igual Sprint 2)
 * ================================================================ */
ResultadoSessao calcular_sessao(Sessao *s, const Eletroposto *ep) {
    ResultadoSessao r;
    float dem;
    memset(&r, 0, sizeof(r));

    /* Energia e tempo: regra unica, compartilhada com iniciar_sessao_vaga() */
    r.energia_kwh = energia_necessaria_kwh(s);
    r.tempo_min   = tempo_recarga_min(r.energia_kwh, s->potencia_real_kw);

    /* Tarifa base do turno: regra unica, compartilhada com gerar_sessoes_demo() */
    r.tarifa_base = tarifa_do_turno(s->turno);
    strncpy(r.descricao_turno, str_turno_detalhado(s->turno),
            sizeof(r.descricao_turno) - 1);

    /* Camadas de desconto, acumuladas em desconto_pct */
    r.desconto_pct = desconto_fidelidade(s->tipo_usuario);
    if (r.desconto_pct > 0.0f) r.tem_fidelidade = 1;

    if (s->minutos_reserva >= 120) {
        r.desconto_pct += DESCONTO_RESERVA_ALT;
        r.tem_reserva   = 1;
    } else if (s->minutos_reserva >= 30) {
        r.desconto_pct += DESCONTO_RESERVA_MED;
        r.tem_reserva   = 1;
    }

    dem = calcular_demanda_total(ep);
    if (dem / DEMANDA_MAX_TOTAL_KW > 0.80f) {
        r.desconto_pct += DESCONTO_ALTA_DEMANDA;
        printf("\n  [TARIFA] Alta demanda — sobretaxa de +10%% aplicada.\n");
    }

    r.tarifa_final = r.tarifa_base * (1.0f - r.desconto_pct);
    if (r.tarifa_final < 0.10f) r.tarifa_final = 0.10f;
    r.custo_reais  = r.energia_kwh * r.tarifa_final;
    return r;
}

/* ================================================================
 *  PAINEL E RELATÓRIO (Sprint 2 mantidos)
 * ================================================================ */
void exibir_painel_vagas(const Eletroposto *ep) {
    int i;
    sep(1);
    printf("  PAINEL — %02dh  |  Demanda: %.1f/%.0f kW\n",
           ep->hora_sistema, ep->demanda_atual_kw, DEMANDA_MAX_TOTAL_KW);
    sep(1);

    float pct = (ep->demanda_atual_kw / DEMANDA_MAX_TOTAL_KW) * 100.0f;
    int   b   = (int)(pct / 5.0f);
    printf("  Rede: [");
    for (i = 0; i < 20; i++) printf(i < b ? (i < 14 ? "#" : "!") : ".");
    printf("]  %.0f%%\n\n", pct);

    sep(0);
    for (i = 0; i < MAX_VAGAS; i++) {
        const Vaga *v = &ep->vagas[i];
        printf("  V%d [%-10s]", v->numero_vaga, str_status(v->status));
        if (v->status == VAGA_OCUPADA) {
            float rest = calcular_tempo_restante_min(v);
            printf("  %s | %s | %.1f kW | ~%.0f min restantes",
                   v->sessao.id_usuario, v->sessao.modelo_veiculo,
                   v->sessao.potencia_real_kw,
                   rest < 0.0f ? 0.0f : rest);
            if (v->resultado.teve_reducao_potencia) printf(" [REDUZIDO]");
        } else if (v->status == VAGA_CONCLUIDA) {
            printf("  %s | %.2f kWh | R$ %.2f",
                   v->sessao.modelo_veiculo,
                   v->resultado.energia_kwh, v->resultado.custo_reais);
        } else {
            printf("  --");
        }
        printf("\n");
    }
    sep(0);
    printf("\n");
}

void exibir_relatorio_consolidado(const Eletroposto *ep) {
    int i;
    float te = 0.0f, tc = 0.0f;
    int   ts = 0;

    sep(1);
    printf("  RELATORIO CONSOLIDADO\n");
    sep(1);

    for (i = 0; i < MAX_VAGAS; i++) {
        const Vaga *v = &ep->vagas[i];
        if (v->status == VAGA_LIVRE) continue;
        ts++;
        te += v->resultado.energia_kwh;
        tc += v->resultado.custo_reais;
        printf("  V%d | %-12s | %-20s | %5.2f kWh | R$ %6.2f | %s\n",
               v->numero_vaga, v->sessao.id_usuario, v->sessao.modelo_veiculo,
               v->resultado.energia_kwh, v->resultado.custo_reais,
               str_status(v->status));
    }

    if (ts == 0) printf("  Nenhuma sessao registrada.\n");
    else {
        sep(0);
        printf("  TOTAL: %d sessao(oes) | %.2f kWh | R$ %.2f\n", ts, te, tc);
        printf("  CO2 evitado estimado: %.1f kg\n", te * 0.233f);
    }
    sep(1);
    printf("\n");
}

void simular_progresso(const Sessao *s, const ResultadoSessao *r) {
    int i, p = 10;
    float kstep = r->energia_kwh / p;
    float bstep = (s->pct_bateria_alvo - s->pct_bateria_atual) / p;
    float ka = 0.0f, bat = s->pct_bateria_atual;

    printf("\n");
    sep(1);
    printf("  SIMULANDO RECARGA (%.1f kW)...\n", s->potencia_real_kw);
    sep(0);

    for (i = 1; i <= p; i++) {
        ka  += kstep;
        bat += bstep;
        int fill = (i * 20) / p;
        printf("  %2d/%2d  [", i, p);
        for (int b = 0; b < 20; b++) printf(b < fill ? "#" : ".");
        printf("]  %5.2f kWh  %5.1f%%\n", ka, bat);
    }
}

void exibir_relatorio(const Sessao *s, const ResultadoSessao *r) {
    printf("\n");
    sep(1);
    printf("  RELATORIO DE SESSAO\n");
    sep(1);
    printf("  Veiculo : %s | %s (%s)\n",
           s->modelo_veiculo, s->id_usuario, str_tipo(s->tipo_usuario));
    printf("  Bateria : %.0f%% -> %.0f%% | %.2f kWh @ %.1f kW\n",
           s->pct_bateria_atual, s->pct_bateria_alvo,
           r->energia_kwh, s->potencia_real_kw);
    printf("  Tempo   : %.0f min | %02dh — %s\n",
           r->tempo_min, s->hora_inicio, r->descricao_turno);
    if (r->teve_reducao_potencia)
        printf("  Potencia reduzida de %.1f kW para %.1f kW\n",
               s->potencia_kw, s->potencia_real_kw);
    sep(0);
    printf("  Tarifa  : R$ %.2f/kWh", r->tarifa_base);
    if (r->desconto_pct != 0.0f)
        printf(" | Ajuste: %.0f%% | Final: R$ %.2f/kWh",
               r->desconto_pct * 100.0f, r->tarifa_final);
    printf("\n");
    sep(0);
    printf("  CUSTO TOTAL: R$ %.2f\n", r->custo_reais);
    sep(1);
    printf("\n");
}

/* ================================================================
 *  OCPP (Sprint 2 mantido)
 * ================================================================ */
void ocpp_log(LogOCPP *log, const char *tipo, const char *msg) {
    if (log->total >= MAX_LOG) return;
    snprintf(log->linhas[log->total++], 199, "[OCPP][%-18s] %s", tipo, msg);
    printf("  [OCPP][%-18s] %s\n", tipo, msg);
}

void ocpp_boot_notification(Eletroposto *ep) {
    char buf[200];
    ocpp_log(&ep->log, "BootNotification",
        "{\"chargePointModel\":\"GoodWe-AC22\",\"chargePointVendor\":\"GoodWe\"}");
    ocpp_log(&ep->log, "BootNotification.conf",
        "{\"status\":\"Accepted\"}");
    snprintf(buf, sizeof(buf),
        "{\"connectorId\":0,\"status\":\"Available\",\"vagas\":%d}", MAX_VAGAS);
    ocpp_log(&ep->log, "StatusNotification", buf);
    printf("\n");
}

void ocpp_status_notification(Eletroposto *ep, int v, StatusVaga st) {
    char buf[200];
    const char *s = (st == VAGA_OCUPADA) ? "Charging" :
                    (st == VAGA_LIVRE)   ? "Available" : "Finishing";
    snprintf(buf, sizeof(buf),
        "{\"connectorId\":%d,\"status\":\"%s\"}", v+1, s);
    ocpp_log(&ep->log, "StatusNotification", buf);
}

void ocpp_start_transaction(Eletroposto *ep, int v, const Sessao *s) {
    char buf[200];
    snprintf(buf, sizeof(buf),
        "{\"connectorId\":%d,\"idTag\":\"%s\",\"meterStart\":0}", v+1, s->id_usuario);
    ocpp_log(&ep->log, "StartTransaction", buf);
    snprintf(buf, sizeof(buf),
        "{\"idTagInfo\":{\"status\":\"Accepted\"},\"transactionId\":%d%d}", v+1, s->hora_inicio);
    ocpp_log(&ep->log, "StartTransaction.conf", buf);
}

void ocpp_stop_transaction(Eletroposto *ep, int v, const ResultadoSessao *r) {
    char buf[200];
    snprintf(buf, sizeof(buf),
        "{\"connectorId\":%d,\"meterStop\":%.0f,\"custo_reais\":%.2f}",
        v+1, r->energia_kwh * 1000.0f, r->custo_reais);
    ocpp_log(&ep->log, "StopTransaction", buf);
    ocpp_log(&ep->log, "StopTransaction.conf", "{\"status\":\"Accepted\"}");
}

void ocpp_meter_values(Eletroposto *ep, int v, float kwh, float pct) {
    char buf[200];
    snprintf(buf, sizeof(buf),
        "{\"connectorId\":%d,\"meterValue\":[{\"value\":\"%.2f\",\"unit\":\"kWh\"},"
        "{\"value\":\"%.0f\",\"unit\":\"Percent\"}]}", v+1, kwh, pct);
    ocpp_log(&ep->log, "MeterValues", buf);
}

void exibir_log_ocpp(const LogOCPP *log) {
    int i;
    sep(1);
    printf("  LOG OCPP — %d entradas\n", log->total);
    sep(0);
    for (i = 0; i < log->total; i++) printf("  %s\n", log->linhas[i]);
    sep(1);
    printf("\n");
}

/* ================================================================
 *  CHATBOT (Sprint 1 mantido)
 * ================================================================ */
void sessao_para_json(const Sessao *s, const ResultadoSessao *r, char *buf, int sz) {
    snprintf(buf, sz,
        "{\"usuario\":\"%s\",\"tipo\":\"%s\",\"veiculo\":\"%s\","
        "\"cap_kwh\":%.1f,\"bat_ini\":%.0f,\"bat_alvo\":%.0f,"
        "\"energia_kwh\":%.2f,\"pot_kw\":%.1f,\"tempo_min\":%.0f,"
        "\"hora\":%d,\"turno\":\"%s\",\"tarifa_base\":%.2f,"
        "\"tarifa_final\":%.2f,\"desconto_pct\":%.0f,\"custo\":%.2f,"
        "\"fidelidade\":%s,\"reserva\":%s,\"reducao_potencia\":%s}",
        s->id_usuario, str_tipo(s->tipo_usuario), s->modelo_veiculo,
        s->capacidade_bateria_kwh, s->pct_bateria_atual, s->pct_bateria_alvo,
        r->energia_kwh, s->potencia_real_kw, r->tempo_min,
        s->hora_inicio, str_turno(s->turno),
        r->tarifa_base, r->tarifa_final, r->desconto_pct * 100.0f, r->custo_reais,
        r->tem_fidelidade ? "true" : "false",
        r->tem_reserva    ? "true" : "false",
        r->teve_reducao_potencia ? "true" : "false"
    );
}

void construir_system_prompt(const char *json, char *buf, int sz) {
    snprintf(buf, sz,
        "Voce e o ChargeGrid Assistant, especialista em eletropostos GoodWe/FIAP.\n"
        "DADOS DA SESSAO: %s\n"
        "REGRAS: responda so sobre recargas, tarifas e equipamentos GoodWe.\n"
        "Use apenas os dados do JSON. Tom profissional em portugues. Max 3 paragrafos.\n"
        "Tarifas: Off-peak R$0,55 | Normal R$0,85 | Pico R$1,20 | Alta demanda +10%%.\n"
        "Descontos: Premium/Frota -5%% | Reserva 30-119min -5%% | 120+min -10%%.\n"
        "IMPORTANTE: Escreva SEM acentos, SEM cedilha e SEM caracteres especiais. "
        "Use apenas letras simples do alfabeto basico. "
        "Exemplo: use 'recarga' em vez de 'recarrega', 'nao' em vez de 'nao' com til, "
        "'sessao' em vez de 'sessao' com til, 'tarifa' normalmente pois nao tem acento.",
        json
    );
}

int chamar_api_claude(const char *sp, const HistoricoChat *h, char *rb, int sz) {
    const char *key = getenv("OLLAMA_API_KEY");
    if (!key || strlen(key) < 10) return 0;

    char sp_esc[MAX_MSG_LEN * 4];
    escapar_json(sp, sp_esc, sizeof(sp_esc));

    char msgs[MAX_JSON_LEN];
    int  pos = snprintf(msgs, sizeof(msgs),
        "[{\"role\":\"system\",\"content\":\"%s\"}", sp_esc);

    int i;
    for (i = 0; i < h->total; i++) {
        char esc[MAX_MSG_LEN * 2];
        escapar_json(h->mensagens[i].content, esc, sizeof(esc));
        pos += snprintf(msgs+pos, sizeof(msgs)-pos,
            ",{\"role\":\"%s\",\"content\":\"%s\"}",
            h->mensagens[i].role, esc);
    }
    snprintf(msgs+pos, sizeof(msgs)-pos, "]");

    char tmp[256], payload_file[256];
    FILE *fp;

    /* Nomes unicos por processo. Antes eram dois blocos #ifdef
       identicos; agora ambos usam nome_arquivo_temp(). */
    nome_arquivo_temp("cg",         tmp,          sizeof(tmp));
    nome_arquivo_temp("cg_payload", payload_file, sizeof(payload_file));

    /* Grava o corpo da requisicao em arquivo. Isso evita problemas
       de escape de aspas na linha de comando do PowerShell.
       (Antes o mesmo JSON era montado duas vezes: uma num buffer
        de 32 KB que nunca chegava a ser usado, e outra aqui.) */
    fp = fopen(payload_file, "w");
    if (!fp) return 0;
    fprintf(fp, "{\"model\":\"gpt-oss:120b-cloud\","
                "\"temperature\":0.3,\"messages\":%s}", msgs);
    fclose(fp);

    char cmd[MAX_CMD_LEN];
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd),
        "powershell -Command \""
        "$h = @{'Content-Type'='application/json';'Authorization'='Bearer %s'};"
        "$b = Get-Content '%s' -Raw;"
        "$r = Invoke-WebRequest -Uri 'https://ollama.com/v1/chat/completions' "
        "-Method POST -Headers $h -Body $b -UseBasicParsing;"
        "$r.Content | Out-File -FilePath '%s' -Encoding utf8\"",
        key, payload_file, tmp);
#else
    snprintf(cmd, sizeof(cmd),
        "curl -s -X POST https://ollama.com/v1/chat/completions "
        "-H \"Content-Type: application/json\" "
        "-H \"Authorization: Bearer %s\" "
        "-d @%s > %s 2>/dev/null",
        key, payload_file, tmp);
#endif

    int ret = system(cmd);
    remove(payload_file);
    if (ret != 0) { remove(tmp); return 0; }

    FILE *f = fopen(tmp, "r");
    if (!f) { remove(tmp); return 0; }
    char raw[MAX_RESP_LEN];
    int  n = (int)fread(raw, 1, sizeof(raw)-1, f);
    fclose(f);
    remove(tmp);
    if (n <= 0) return 0;
    raw[n] = '\0';
    extrair_resposta_json(raw, rb, sz);
    return strlen(rb) > 0 ? 1 : 0;
}

void extrair_resposta_json(const char *raw, char *buf, int sz) {
    buf[0] = '\0';
    const char *m = strstr(raw, "\"content\":\"");
    if (!m) {
        m = strstr(raw, "\"message\":\"");
        if (m) { m += 11; }
        else return;
    } else { m += 11; }

    int i = 0;
    while (*m && i < sz-1) {
        if (*m == '"' && (m == raw || *(m-1) != '\\')) break;
        if (*m == '\\' && *(m+1)) {
            m++;
            switch (*m) {
                case 'n': buf[i++] = '\n'; break;
                case 't': buf[i++] = '\t'; break;
                case 'r': break;
                case '"': buf[i++] = '"';  break;
                case '\\':buf[i++] = '\\'; break;
                default:  buf[i++] = *m;   break;
            }
        } else buf[i++] = *m;
        m++;
    }
    buf[i] = '\0';
}

void adicionar_mensagem(HistoricoChat *h, const char *role, const char *content) {
    if (h->total >= MAX_HISTORICO) {
        int i;
        for (i = 0; i < MAX_HISTORICO-1; i++) h->mensagens[i] = h->mensagens[i+1];
        h->total = MAX_HISTORICO-1;
    }
    strncpy(h->mensagens[h->total].role,    role,    15);
    strncpy(h->mensagens[h->total].content, content, MAX_MSG_LEN-1);
    h->mensagens[h->total].role[15]          = '\0';
    h->mensagens[h->total].content[MAX_MSG_LEN-1] = '\0';
    h->total++;
}

void loop_chatbot(const Sessao *s, const ResultadoSessao *r) {
    HistoricoChat hist;
    hist.total = 0;

    char json[2048], sp[MAX_MSG_LEN * 2];
    char msg[MAX_MSG_LEN], resp[MAX_RESP_LEN];

    sessao_para_json(s, r, json, sizeof(json));
    construir_system_prompt(json, sp, sizeof(sp));

    sep(1);
    printf("\n  ChargeGrid Assistant — %s\n  Digite 'sair' para voltar.\n\n", s->modelo_veiculo);
    sep(0);
    printf("\n  [Assistant] Ola! Tenho os dados da sessao do %s. Como posso ajudar?\n\n", s->modelo_veiculo);

    while (1) {
        printf("  Voce > ");
        int c;
        while ((c = getchar()) == '\n' || c == '\r');
        ungetc(c, stdin);
        if (!fgets(msg, sizeof(msg), stdin)) break;
        int len = (int)strlen(msg);
        if (len > 0 && msg[len-1] == '\n') msg[len-1] = '\0';
        if (strcmp(msg,"sair")==0 || strcmp(msg,"exit")==0) {
            printf("\n  Chat encerrado.\n\n"); break;
        }
        if (!strlen(msg)) continue;

        adicionar_mensagem(&hist, "user", msg);
        printf("\n  [Assistant] ...\n");
        int ok = chamar_api_claude(sp, &hist, resp, sizeof(resp));
        if (ok) {
            adicionar_mensagem(&hist, "assistant", resp);
            printf("\n  [Assistant] ");
            imprimir_resposta(resp);
            printf("\n\n");
        } else {
            printf("\n  [Assistant - OFFLINE] Sem acesso a IA. Verifique OLLAMA_API_KEY.\n\n");
        }
        sep(0);
        printf("\n");
    }
}

/* Imprime texto no terminal com suporte UTF-8 no Windows
 * Tambem remove marcadores de markdown como **negrito** */
void imprimir_resposta(const char *texto) {
#ifdef _WIN32
    /* Converte UTF-8 para Wide String e usa WriteConsoleW */
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    /* Remove ** do markdown antes de imprimir */
    char limpo[MAX_RESP_LEN];
    int i = 0, j = 0;
    while (texto[i] && j < MAX_RESP_LEN - 1) {
        if (texto[i] == '*' && texto[i+1] == '*') {
            i += 2; /* pula ** */
        } else {
            limpo[j++] = texto[i++];
        }
    }
    limpo[j] = '\0';
    /* Converte para wide e imprime */
    wchar_t wbuf[MAX_RESP_LEN];
    int wlen = MultiByteToWideChar(CP_UTF8, 0, limpo, -1, wbuf, MAX_RESP_LEN);
    if (wlen > 0) {
        DWORD written;
        WriteConsoleW(hOut, wbuf, wlen - 1, &written, NULL);
    } else {
        printf("%s", limpo); /* fallback */
    }
#else
    printf("%s", texto);
#endif
}

void escapar_json(const char *in, char *out, int max) {
    int i=0, j=0;
    while (in[i] && j < max-2) {
        switch(in[i]) {
            case '"':  out[j++]='\\'; out[j++]='"';  break;
            case '\\': out[j++]='\\'; out[j++]='\\'; break;
            case '\n': out[j++]='\\'; out[j++]='n';  break;
            case '\r': out[j++]='\\'; out[j++]='r';  break;
            case '\t': out[j++]='\\'; out[j++]='t';  break;
            default:   out[j++]=in[i]; break;
        }
        i++;
    }
    out[j]='\0';
}

/* ================================================================
 *  UTILITÁRIOS
 * ================================================================ */
void exibir_banner(void) {
    printf("\n%s\n", SEP);
    printf("  ChargeGrid Intelligence — Sprint 3\n");
    printf("  EV Challenge 2026 | FIAP x GoodWe | Data Structures\n");
    printf("  Vetor de estruturas | Busca linear e binaria | Bubble e Insertion Sort\n");
    printf("  %d vagas | %.0f kW limite | ate %d sessoes no historico\n",
           MAX_VAGAS, DEMANDA_MAX_TOTAL_KW, MAX_SESSOES);
    printf("%s\n\n", SEP);
}

void sep(int t) { printf("%s\n", t ? SEP : LINE); }

/* Procura uma vaga para uma nova recarga.
 *
 * CORRECAO: da preferencia a uma vaga nunca usada (VAGA_LIVRE) para
 * preservar o maximo de historico visual no painel. Se nao houver,
 * reaproveita a vaga concluida ha mais tempo (a primeira encontrada).
 * iniciar_sessao_vaga() sobrescreve sessao e resultado por completo,
 * entao nao sobra nenhum dado da recarga anterior.
 *
 * So retorna -1 quando TODAS as vagas estao realmente carregando. */
int encontrar_vaga_livre(const Eletroposto *ep) {
    int i;
    for (i = 0; i < MAX_VAGAS; i++)
        if (ep->vagas[i].status == VAGA_LIVRE) return i;
    for (i = 0; i < MAX_VAGAS; i++)
        if (ep->vagas[i].status == VAGA_CONCLUIDA) return i;
    return -1;
}

TipoUsuario menu_tipo_usuario(void) {
    int op, t = 0;
    do {
        if (t++) printf("  Invalido.\n");
        printf("\n  Tipo: 1-Comum  2-Premium  3-Frota > ");
        scanf("%d", &op);
    } while (op < 1 || op > 3);
    return (TipoUsuario)op;
}

TurnoTarifario definir_turno(int h) {
    if (h >= 0  && h < 6)  return TURNO_OFFPEAK;
    if (h >= 18 && h < 22) return TURNO_PICO;
    return TURNO_NORMAL;
}

/* Descarta o que sobrou da linha atual depois de um scanf malsucedido.
 *
 * CORRECAO: o codigo anterior usava "while (getchar() != '\n');".
 * Quando a entrada acaba (EOF — arquivo redirecionado, Ctrl+D,
 * Ctrl+Z), getchar() passa a devolver EOF indefinidamente e o '\n'
 * nunca chega: o programa ficava preso em laco infinito.
 *
 * Retorna 1 se conseguiu chegar ao fim da linha, 0 se a entrada
 * terminou. */
int descartar_linha(void) {
    int c;
    while ((c = getchar()) != '\n') {
        if (c == EOF) return 0;
    }
    return 1;
}

float ler_float(const char *msg, float mn, float mx) {
    float v; int t = 0;
    while (1) {
        if (t++ >= MAX_TENTATIVAS) { printf("  Usando %.1f\n", mn); return mn; }
        printf("%s", msg);
        if (scanf("%f", &v) == 1 && v >= mn && v <= mx) return v;
        if (!descartar_linha()) {
            printf("\n  [ENTRADA] Fim da entrada. Usando %.1f\n", mn);
            return mn;
        }
        printf("  Invalido [%.1f-%.1f]\n", mn, mx);
    }
}

int ler_int(const char *msg, int mn, int mx) {
    int v; int t = 0;
    while (1) {
        if (t++ >= MAX_TENTATIVAS) { printf("  Usando %d\n", mn); return mn; }
        printf("%s", msg);
        if (scanf("%d", &v) == 1 && v >= mn && v <= mx) return v;
        if (!descartar_linha()) {
            printf("\n  [ENTRADA] Fim da entrada. Usando %d\n", mn);
            return mn;
        }
        printf("  Invalido [%d-%d]\n", mn, mx);
    }
}

const char *str_tipo(TipoUsuario t) {
    switch(t) {
        case USUARIO_PREMIUM: return "Premium";
        case USUARIO_FROTA:   return "Frota";
        default:              return "Comum";
    }
}

const char *str_turno(TurnoTarifario t) {
    switch(t) {
        case TURNO_OFFPEAK: return "Off-peak";
        case TURNO_PICO:    return "Pico";
        default:            return "Normal";
    }
}

const char *str_status(StatusVaga s) {
    switch(s) {
        case VAGA_OCUPADA:   return "OCUPADA";
        case VAGA_CONCLUIDA: return "CONCLUIDA";
        default:             return "LIVRE";
    }
}

const char *str_status_html(StatusVaga s) {
    switch(s) {
        case VAGA_OCUPADA:   return "ocupada";
        case VAGA_CONCLUIDA: return "concluida";
        default:             return "livre";
    }
}
