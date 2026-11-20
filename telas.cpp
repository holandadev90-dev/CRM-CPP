#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

// Define o STB Image com guardas corretas
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif

#include "crm_produtos.h"
#include "telas.h"
#include "modulos.h"
#include "imgui.h"
#include "banco.h"
#include "cliente.h"
#include <ctime>
#include <map>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <iostream>
#include "sqlite3.h"
#include <algorithm>
#include <cctype>
#include <windows.h>
#include <commdlg.h>
#include <cstring>
#include <shellapi.h>

// Mantenha apenas UMA inclusão do stb_image (use o caminho correto do seu projeto)
#include "include/stb_image.h"

struct ItemComandaLancado
{
    int produto_id;
    std::string nome;
    float preco_unitario;
    int quantidade;
    float subtotal;
};

// Variáveis globais para armazenar o estado do sistema
static std::map<int, std::vector<ItemComandaLancado>> g_ComandasPorCadeira;

// Cache de texturas para carregar cada arquivo apenas uma vez
static std::map<std::string, GLuint> g_cacheTexturasBarbeiros;

static int itemParaExcluirID = -1;
static bool dispararModalExcluirEstoque = false;
static GLuint g_TextureQrCode = 0;
static std::string g_CaminhoAtualCarregado = "";

// ============================================================
// DEFINIÇÃO DAS VARIÁVEIS GLOBAIS DE FUNDOS (Fora de qualquer função)
// ============================================================
GLuint texturasFundos[4] = {0, 0, 0, 0};
int temaFundoAtual = 0;

void RenderizarModalExcluirEstoque(sqlite3 *db, int usuario_id);

std::string URLEncode(const std::string &value)
{
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : value)
    {
        if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            escaped << c;
        }
        else if (c == ' ')
        {
            escaped << "%20";
        }
        else if (c == '\n')
        {
            escaped << "%0A";
        }
        else
        {
            escaped << '%' << std::setw(2) << (int)(unsigned char)c;
        }
    }
    return escaped.str();
}

struct BarbeiroInfo
{
    int id;
    std::string nome;
};

struct RegistroHistorico
{
    std::string cliente;
    std::string servico;
    float valor;
    std::string data;
};

struct PixConfig
{
    std::string tipoChave;
    std::string chave;
    std::string nomeTitular;
};

// Variável para armazenar a lista na memória e o faturamento total acumulado
std::vector<RegistroHistorico> listaHistoricoCadeira;
float faturamentoTotalHistorico = 0.0f;

// Função auxiliar para obter a data atual formatada (YYYY-MM-DD) dinamicamente
std::string ObterDataAtualTelas()
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#if defined(_MSC_VER) || defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

struct ItemEstoqueCombo
{
    int id;
    std::string nome;
    float preco;
    int qtdEstoque;
};

float faturamentoDiarioCadeira = 0.0f;

void RenderizarJanelaGerenciamento(bool *p_open)
{
    if (p_open && *p_open)
    {
        ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Gerenciamento de Produtos e Servicos", p_open))
        {
            if (ImGui::BeginTabBar("ManagerTabs"))
            {
                if (ImGui::BeginTabItem("Clientes / Nichos"))
                {
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.5f, 1.0f), "Painel de Controle - Estabelecimentos Cadastrados");
                    ImGui::Separator();
                    ImGui::Spacing();

                    std::vector clientes = obterListaClientes();

                    if (clientes.empty())
                    {
                        ImGui::Text("Nenhum cliente cadastrado no sistema no momento.");
                    }
                    else
                    {
                        if (ImGui::BeginTable("TabelaClientesCRM", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
                        {
                            ImGui::TableSetupColumn("ID");
                            ImGui::TableSetupColumn("Usuario");
                            ImGui::TableSetupColumn("Nome de Exibicao");
                            ImGui::TableSetupColumn("Tipo de Negocio");
                            ImGui::TableSetupColumn("Acoes");
                            ImGui::TableHeadersRow();

                            for (const auto &c : clientes)
                            {
                                ImGui::TableNextRow();

                                ImGui::TableSetColumnIndex(0);
                                ImGui::Text("%d", c.id);

                                ImGui::TableSetColumnIndex(1);
                                ImGui::Text("%s", c.username.c_str());

                                ImGui::TableSetColumnIndex(2);
                                ImGui::Text("%s", c.nomeExibicao.c_str());

                                ImGui::TableSetColumnIndex(3);
                                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", c.tipoNegocio.c_str());

                                ImGui::TableSetColumnIndex(4);
                                std::string btnNome = "Editar##" + std::to_string(c.id);
                                if (ImGui::Button(btnNome.c_str()))
                                {
                                    // Ação futura de edição
                                }
                            }
                            ImGui::EndTable();
                        }
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Produtos"))
                {
                    ImGui::Text("Gerencie seus produtos aqui...");
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Servicos"))
                {
                    ImGui::Text("Gerencie seus servicos aqui...");
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }
}

void RenderizarJanelaAvancada(bool *p_open)
{
    if (p_open && *p_open)
    {
        ImGui::SetNextWindowSize(ImVec2(700, 450), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Configuracoes Avancadas e Relatorios", p_open))
        {
            if (ImGui::BeginTabBar("AdvancedTabs"))
            {
                if (ImGui::BeginTabItem("Historico de Clientes"))
                {
                    ImGui::Text("Aqui voce podera puxar o historico detalhado do cliente.");
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Gerenciar / Excluir Clientes"))
                {
                    ImGui::Text("Area para exclusao de clientes por ID ou Nome.");
                    static char buscaCliente[64] = "";
                    ImGui::InputText("ID ou Nome do Cliente", buscaCliente, IM_ARRAYSIZE(buscaCliente));

                    if (ImGui::Button("Excluir Cliente"))
                    {
                        // TODO: Chamar funcao de exclusão do banco
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Relatorios"))
                {
                    ImGui::Text("Geracao de relatorios e metricas do sistema...");
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Reclamacoes / Suporte"))
                {
                    ImGui::Text("Registro e acompanhamento de reclamacoes.");
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }
}

// ==========================================
// 1. ESTRUTURA DE ESTADO DA TELA (CLEAN ARCHITECTURE)
// ==========================================

struct CadeirasContext
{
    // Buffers de Input
    char novoNomeBarbeiro[128] = "";
    char editNomeBarbeiro[128] = "";
    char nomeCliente[128] = "";
    char telefoneCliente[128] = "";
    char obsServico[256] = "";
    char senhaExclusao[64] = "";
    char nomeClienteEncaixe[128] = "";
    char servicoEncaixe[128] = "Corte";
    char caminhoFotoInput[512] = "";

    // Seletores e Valores
    int dataComboIndex = 0;
    int horaIndex = 8;    // 08:00
    int minIndex = 0;     // :00
    int filtroAgenda = 0; // 0 = Hoje, 1 = Próximos 7 Dias, 2 = Todos
    int usuarioLogadoId = 1;
    float valorServico = 40.0f;
    float valorEncaixe = 40.0f;

    // Controle de Cadeiras
    int cadeiraSelecionadaID = -1;
    int cadeiraSelecionadaParaEncaixe = -1;

    // Flags de Modais
    bool modalEncaixe = false;
    bool modalGerenciar = false;
    bool modalEditar = false;
    bool modalExcluir = false;
    bool modalAdicionar = false;
    bool mostrandoHistorico = false;
    bool modalAlertaAtraso = false;
    bool modalFechamento = false;
    bool modalAdicionarFoto = false;

    // Mensagens
    std::string mensagemErroConflito = "";
    std::string erroSenhaExclusao = "";

    // Configuração do Pix
    char chavePixMain[100] = "";     // Ex: "123.456.789-00" ou "contato@barbearia.com"
    char nomeTitularPix[100] = "";   // Ex: "Barbearia do João Ltda"
    bool modalConfigPix = false;     // Controla se o modal de cadastro está aberto
    bool modalReceberPix = false;    // Controla o modal de cobrança/checkout
    float valorCobrancaAtual = 0.0f; // Valor que vai aparecer no QR Code
    char caminhoQrCode[256] = "";
};

GLuint ObterOuCarregarTextura(const std::string &caminhoArquivo)
{
    if (caminhoArquivo.empty())
        return 0;

    // Se já estiver no cache, retorna a ID da textura OpenGL existente
    if (g_cacheTexturasBarbeiros.find(caminhoArquivo) != g_cacheTexturasBarbeiros.end())
    {
        return g_cacheTexturasBarbeiros[caminhoArquivo];
    }

    int larg = 0, alt = 0, canais = 0;
    unsigned char *data = stbi_load(caminhoArquivo.c_str(), &larg, &alt, &canais, 4);
    if (!data)
        return 0; // Arquivo não encontrado ou inválido

    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, larg, alt, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    g_cacheTexturasBarbeiros[caminhoArquivo] = textureID;
    return textureID;
}
// Instância estática para manter o estado entre os frames
static CadeirasContext ctx;

// Função utilitária para fechar todas as janelas e modais ativos na tela

void FecharTodasAsJanelasExclusivas()
{
    // 1. Reseta a tela de estoque (esta variável existe globalmente)
    extern bool showProdutosWindow;
    showProdutosWindow = false;

    // 2. Modais e janelas do contexto global (Pix, Agendamentos, Edição)
    ctx.modalReceberPix = false;
    ctx.modalConfigPix = false;
    ctx.modalAdicionar = false;
    ctx.modalEditar = false;
    ctx.modalEncaixe = false;
    ctx.modalExcluir = false;
    ctx.modalGerenciar = false;

    // 3. Reseta seleções e histórico interno
    ctx.cadeiraSelecionadaID = 0;
    ctx.cadeiraSelecionadaParaEncaixe = 0;
    ctx.mostrandoHistorico = false;
}

// ==========================================
// 2. FUNÇÕES AUXILIARES E RENDERIZAÇÃO DE MODAIS
// ==========================================

void RenderizarModalAdicionarCadeira(sqlite3 *db)
{
    if (ctx.modalAdicionar)
    {
        ImGui::OpenPopup("ModalAdicionarBarbeiroUnico");
        ctx.modalAdicionar = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("ModalAdicionarBarbeiroUnico", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Cadastrar Novo Barbeiro / Cadeira");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::InputText("Nome do Barbeiro##inputNovoBarb", ctx.novoNomeBarbeiro, IM_ARRAYSIZE(ctx.novoNomeBarbeiro));
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.3f, 1.0f));
        if (ImGui::Button("Salvar Cadeira##btnSalvarBarb", ImVec2(130, 30)))
        {
            if (strlen(ctx.novoNomeBarbeiro) > 0)
            {
                const char *sqlInsert = "INSERT INTO barbeiros (usuario_id, nome, ativo) VALUES (1, ?, 1);";
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, sqlInsert, -1, &stmt, nullptr) == SQLITE_OK)
                {
                    sqlite3_bind_text(stmt, 1, ctx.novoNomeBarbeiro, -1, SQLITE_TRANSIENT);
                    sqlite3_step(stmt);
                    sqlite3_finalize(stmt);
                }
                memset(ctx.novoNomeBarbeiro, 0, sizeof(ctx.novoNomeBarbeiro));
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        if (ImGui::Button("Cancelar##btnCancelBarb", ImVec2(100, 30)))
        {
            memset(ctx.novoNomeBarbeiro, 0, sizeof(ctx.novoNomeBarbeiro));
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void RenderizarModalEditarCadeira(sqlite3 *db)
{
    if (ctx.modalEditar)
    {
        ImGui::OpenPopup("ModalEditarBarbeiroUnico");
        ctx.modalEditar = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("ModalEditarBarbeiroUnico", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Editar Nome do Barbeiro (ID: %d)", ctx.cadeiraSelecionadaID);
        ImGui::Separator();
        ImGui::InputText("Novo Nome##inputEditBarb", ctx.editNomeBarbeiro, IM_ARRAYSIZE(ctx.editNomeBarbeiro));
        ImGui::Spacing();

        if (ImGui::Button("Atualizar Nome##btnUpdateBarb", ImVec2(130, 30)))
        {
            if (strlen(ctx.editNomeBarbeiro) > 0)
            {
                const char *sqlUpdate = "UPDATE barbeiros SET nome = ? WHERE id = ?;";
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, sqlUpdate, -1, &stmt, nullptr) == SQLITE_OK)
                {
                    sqlite3_bind_text(stmt, 1, ctx.editNomeBarbeiro, -1, SQLITE_TRANSIENT);
                    sqlite3_bind_int(stmt, 2, ctx.cadeiraSelecionadaID);
                    sqlite3_step(stmt);
                    sqlite3_finalize(stmt);
                }
                memset(ctx.editNomeBarbeiro, 0, sizeof(ctx.editNomeBarbeiro));
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancelar##btnCancelEdit", ImVec2(100, 30)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void RenderizarModalEncaixeRapido(sqlite3 *db)
{
    if (ctx.modalEncaixe)
    {
        ImGui::OpenPopup("ModalEncaixeRapidoUnico");
        ctx.modalEncaixe = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("ModalEncaixeRapidoUnico", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Registrar Cliente de Balcão / Encaixe");
        ImGui::Separator();

        bool bloqueadoPorHorarioProximo = false;

        if (ctx.cadeiraSelecionadaParaEncaixe != -1)
        {
            std::string proxAge = buscarProximoAgendamento(db, ctx.cadeiraSelecionadaParaEncaixe, "");
            if (!proxAge.empty() && proxAge != "Nenhum agendamento próximo")
            {
                std::string sqlChecarMinutos =
                    "SELECT (julianday(data_hora) - julianday('now', 'localtime')) * 24 * 60 AS minutos_restantes "
                    "FROM agendamentos "
                    "WHERE barbeiro_id = ? AND date(data_hora) = date('now', 'localtime') "
                    "ORDER BY data_hora ASC LIMIT 1;";

                sqlite3_stmt *stmtMin = nullptr;
                if (sqlite3_prepare_v2(db, sqlChecarMinutos.c_str(), -1, &stmtMin, nullptr) == SQLITE_OK)
                {
                    sqlite3_bind_int(stmtMin, 1, ctx.cadeiraSelecionadaParaEncaixe);
                    if (sqlite3_step(stmtMin) == SQLITE_ROW)
                    {
                        double minutosRestantes = sqlite3_column_double(stmtMin, 0);

                        if (minutosRestantes >= -5.0 && minutosRestantes <= 10.0)
                        {
                            bloqueadoPorHorarioProximo = true;
                        }
                    }
                    sqlite3_finalize(stmtMin);
                }

                ImGui::Spacing();
                if (bloqueadoPorHorarioProximo)
                {
                    ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "🚫 ENCAIXE BLOQUEADO: Agendamento em menos de 10 min!");
                    ImGui::TextWrapped("Próximo: %s", proxAge.c_str());
                    ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.4f, 1.0f), "Reserva mantida para o cliente que agendou o horário.");
                }
                else
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.0f, 1.0f), "Atenção: Existe agendamento próximo!");
                    ImGui::TextWrapped("Próximo: %s", proxAge.c_str());
                    ImGui::TextDisabled("Certifique-se de que o encaixe não irá atrasar este horário.");
                }
                ImGui::Separator();
            }
        }

        ImGui::InputText("Cliente##enc", ctx.nomeClienteEncaixe, sizeof(ctx.nomeClienteEncaixe));
        ImGui::InputText("Serviço##enc", ctx.servicoEncaixe, sizeof(ctx.servicoEncaixe));
        ImGui::InputFloat("Valor (R$)##enc", &ctx.valorEncaixe);
        ImGui::Spacing();

        bool nomeValido = (strlen(ctx.nomeClienteEncaixe) > 0);
        bool podeConfirmar = nomeValido && !bloqueadoPorHorarioProximo;

        if (!podeConfirmar)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("Confirmar encaixe", ImVec2(150, 0)))
        {
            if (ctx.cadeiraSelecionadaParaEncaixe != -1 && podeConfirmar)
            {
                alternarStatusAtendimento(db, ctx.cadeiraSelecionadaParaEncaixe);

                // Grava na tabela 'agendamentos' para a Cadeira/Comanda gerenciar
                // O historico sera gravado apenas quando o atendimento for finalizado ("Feito")
                const char *sqlAgendamento = "INSERT INTO agendamentos (usuario_id, barbeiro_id, cliente_nome, servico, valor, data_hora) VALUES (1, ?, ?, ?, ?, datetime('now', 'localtime'));";
                sqlite3_stmt *stmtAg;
                if (sqlite3_prepare_v2(db, sqlAgendamento, -1, &stmtAg, nullptr) == SQLITE_OK)
                {
                    sqlite3_bind_int(stmtAg, 1, ctx.cadeiraSelecionadaParaEncaixe);
                    sqlite3_bind_text(stmtAg, 2, ctx.nomeClienteEncaixe, -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text(stmtAg, 3, ctx.servicoEncaixe, -1, SQLITE_TRANSIENT);
                    sqlite3_bind_double(stmtAg, 4, ctx.valorEncaixe);
                    sqlite3_step(stmtAg);
                    sqlite3_finalize(stmtAg);
                }

                memset(ctx.nomeClienteEncaixe, 0, sizeof(ctx.nomeClienteEncaixe));
                memset(ctx.servicoEncaixe, 0, sizeof(ctx.servicoEncaixe));
                ctx.valorEncaixe = 0.0f;
                ctx.cadeiraSelecionadaParaEncaixe = -1;
                ImGui::CloseCurrentPopup();
            }
        }

        if (!podeConfirmar)
        {
            ImGui::EndDisabled();
            ImGui::SameLine();

            if (!nomeValido)
            {
                ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "⚠️ Digite o nome do cliente!");
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancelar##enc", ImVec2(100, 0)))
        {
            ctx.cadeiraSelecionadaParaEncaixe = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void RenderizarModalExcluirCadeira(sqlite3 *db)
{
    if (ctx.modalExcluir)
    {
        ImGui::OpenPopup("ModalExcluirBarbeiroUnico");
        ctx.modalExcluir = false;
    }

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("ModalExcluirBarbeiroUnico", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Seguranca: Confirmar Exclusao");
        ImGui::Separator();
        ImGui::Text("Deseja mesmo excluir a Cadeira ID: %d?", ctx.cadeiraSelecionadaID);
        ImGui::Text("Insira a senha da sua conta CRM para confirmar:");

        ImGui::InputText("Senha##inputDelPass", ctx.senhaExclusao, IM_ARRAYSIZE(ctx.senhaExclusao), ImGuiInputTextFlags_Password);

        if (!ctx.erroSenhaExclusao.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", ctx.erroSenhaExclusao.c_str());
        }

        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("Confirmar Exclusao##btnConfirmDel", ImVec2(150, 30)))
        {
            if (strlen(ctx.senhaExclusao) > 0)
            {
                // Prevenção de SQL Injection na validação de senha
                std::string sqlValida = "SELECT COUNT(*) FROM usuarios WHERE senha = ?;";
                sqlite3_stmt *stmtValida = nullptr;
                bool senhaCorreta = false;

                if (sqlite3_prepare_v2(db, sqlValida.c_str(), -1, &stmtValida, nullptr) == SQLITE_OK)
                {
                    sqlite3_bind_text(stmtValida, 1, ctx.senhaExclusao, -1, SQLITE_TRANSIENT);
                    if (sqlite3_step(stmtValida) == SQLITE_ROW)
                    {
                        if (sqlite3_column_int(stmtValida, 0) > 0)
                            senhaCorreta = true;
                    }
                    sqlite3_finalize(stmtValida);
                }

                if (senhaCorreta)
                {
                    char *erro = nullptr;
                    std::string sqlDelAgend = "DELETE FROM agendamentos WHERE barbeiro_id = " + std::to_string(ctx.cadeiraSelecionadaID) + ";";
                    sqlite3_exec(db, sqlDelAgend.c_str(), 0, 0, &erro);

                    std::string sqlDelBarb = "DELETE FROM barbeiros WHERE id = " + std::to_string(ctx.cadeiraSelecionadaID) + ";";
                    sqlite3_exec(db, sqlDelBarb.c_str(), 0, 0, &erro);

                    memset(ctx.senhaExclusao, 0, sizeof(ctx.senhaExclusao));
                    ctx.erroSenhaExclusao.clear();
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    ctx.erroSenhaExclusao = "Senha incorreta!";
                }
            }
            else
            {
                ctx.erroSenhaExclusao = "Digite a senha!";
            }
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        if (ImGui::Button("Cancelar##btnCancelDel", ImVec2(100, 30)))
        {
            memset(ctx.senhaExclusao, 0, sizeof(ctx.senhaExclusao));
            ctx.erroSenhaExclusao.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void RenderizarModalGerenciarCadeira(sqlite3 *db)
{
    if (ctx.modalGerenciar)
    {
        ImGui::OpenPopup("ModalGerenciarCadeiraUnico");
        ctx.modalGerenciar = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(900, 710), ImGuiCond_Appearing); // Espaço ideal para respirar

    if (ImGui::BeginPopupModal("ModalGerenciarCadeiraUnico", NULL, ImGuiWindowFlags_None))
    {
        time_t agora = time(0);
        tm *ltm = localtime(&agora);

        char bufferDataHoje[11];
        sprintf(bufferDataHoje, "%04d-%02d-%02d", 1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday);
        std::string dataHojeStr(bufferDataHoje);

        int diaSemanaAtual = ltm->tm_wday;
        int diasAteSegunda = (diaSemanaAtual == 0) ? 6 : (diaSemanaAtual - 1);
        time_t segundaFeiraEstaSemana = agora - (diasAteSegunda * 24 * 60 * 60);

        char diasVetorStr[7][64];
        const char *diasArr[7];
        static char datasPuras[7][11];
        const char *nomesDiasSemana[] = {"Dom", "Seg", "Ter", "Qua", "Qui", "Sex", "Sáb"};

        for (int i = 0; i < 7; i++)
        {
            time_t diaDaSemana = segundaFeiraEstaSemana + (i * 24 * 60 * 60);
            tm *ltmDia = localtime(&diaDaSemana);
            sprintf(datasPuras[i], "%04d-%02d-%02d", 1900 + ltmDia->tm_year, 1 + ltmDia->tm_mon, ltmDia->tm_mday);

            std::string dataStrLoop = datasPuras[i];
            int wday = ltmDia->tm_wday;
            std::string rotuloDia = dataStrLoop + " (" + nomesDiasSemana[wday] + ")";

            if (dataStrLoop < dataHojeStr)
                rotuloDia += " (X - Passado)";
            else if (dataStrLoop == dataHojeStr)
                rotuloDia += " (Hoje - Atencao!)";
            else
                rotuloDia += " (Disponivel)";

            snprintf(diasVetorStr[i], sizeof(diasVetorStr[i]), "%s", rotuloDia.c_str());
            diasArr[i] = diasVetorStr[i];
        }

        // --- BARRA SUPERIOR DE NAVEGAÇÃO ---
        if (BotaoCancelar("← Voltar / Fechar", ImVec2(160, 35)))
        {
            ctx.mostrandoHistorico = false;
            ImGui::CloseCurrentPopup();
        }

        float larguraBotaoHistorico = 220.0f;
        ImGui::SameLine(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - larguraBotaoHistorico);

        if (ImGui::Button(ctx.mostrandoHistorico ? "Ver Agenda Atual##btnHist" : "Ver Histórico de Serviços##btnHist", ImVec2(larguraBotaoHistorico, 35)))
        {
            if (!ctx.mostrandoHistorico)
            {
                CarregarHistoricoCadeira(db, ctx.cadeiraSelecionadaID, 30);
            }
            ctx.mostrandoHistorico = !ctx.mostrandoHistorico;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ctx.mostrandoHistorico)
        {
            // --- SEÇÃO DE HISTÓRICO REFINADA ---
            RenderizarTituloModal("Histórico de Serviços Concluídos");
            ImGui::Spacing();

            // Centralizando a barra de pesquisa/filtros do histórico
            float largDisponivel = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX((largDisponivel - 450.0f) * 0.5f);

            static int opcaoPeriodo = 0;
            const char *periodos[] = {"Últimos 30 dias", "Últimos 90 dias", "Todo o Histórico"};

            ImGui::SetNextItemWidth(170.0f);
            if (ImGui::Combo("##PeriodoHist", &opcaoPeriodo, periodos, IM_ARRAYSIZE(periodos)))
            {
                int diasFiltro = 30;
                if (opcaoPeriodo == 1)
                    diasFiltro = 90;
                else if (opcaoPeriodo == 2)
                    diasFiltro = 0;
                CarregarHistoricoCadeira(db, ctx.cadeiraSelecionadaID, diasFiltro);
            }

            ImGui::SameLine();
            static char filtroHistorico[64] = "";
            ImGui::SetNextItemWidth(260.0f);
            ImGui::InputTextWithHint("##BuscarCliHist", "🔍 Buscar cliente ou serviço...", filtroHistorico, IM_ARRAYSIZE(filtroHistorico));
            ImGui::Spacing();

            if (ImGui::BeginTable("TabelaHistoricoCadeiraUnica", 4, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit, ImVec2(0, 360.0f)))
            {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("Cliente", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Serviço / Obs", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Valor", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Data / Hora", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                ImGui::TableHeadersRow();

                for (const auto &item : listaHistoricoCadeira)
                {
                    if (filtroHistorico[0] != '\0')
                    {
                        std::string busca = filtroHistorico;
                        if (item.cliente.find(busca) == std::string::npos && item.servico.find(busca) == std::string::npos)
                        {
                            continue;
                        }
                    }

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(item.cliente.c_str());

                    ImGui::TableSetColumnIndex(1);
                    std::string servicoLimpo = item.servico;
                    size_t posTel = servicoLimpo.find(" - Tel:");
                    if (posTel != std::string::npos)
                        servicoLimpo = servicoLimpo.substr(0, posTel);
                    ImGui::TextUnformatted(servicoLimpo.c_str());

                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "R$ %.2f", item.valor);

                    ImGui::TableSetColumnIndex(3);
                    ImGui::TextDisabled("%s", item.data.c_str());
                }
                ImGui::EndTable();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Rodapé do histórico centralizado
            ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - 620.0f) * 0.5f);
            ImGui::Text("Faturamento de Hoje:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "R$ %.2f", faturamentoDiarioCadeira);
            ImGui::SameLine(0, 30);
            ImGui::Text("Total do Período:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "R$ %.2f", faturamentoTotalHistorico);
            ImGui::SameLine(0, 30);
            ImGui::TextDisabled("Total: %zu serviços", listaHistoricoCadeira.size());
        }
        else
        {
            // --- ABA DE AGENDA E NOVO AGENDAMENTO ---
            char tituloCadeira[128];
            snprintf(tituloCadeira, sizeof(tituloCadeira), "Agenda da Cadeira - Barbeiro ID: %d", ctx.cadeiraSelecionadaID);

            // Título Centralizado com Destaque
            float largAvail = ImGui::GetContentRegionAvail().x;
            float largTitulo = ImGui::CalcTextSize(tituloCadeira).x;
            ImGui::SetCursorPosX((largAvail - largTitulo) * 0.5f);
            ImGui::TextColored(ImVec4(0.3f, 0.85f, 1.0f, 1.0f), "%s", tituloCadeira);
            ImGui::Spacing();

            // CARD DO FORMULÁRIO COM DESIGN FLUIDO E MODERNO
            if (ImGui::BeginChild("CardNovoAgendamento", ImVec2(0, 255), true, ImGuiChildFlags_AlwaysUseWindowPadding))
            {
                // Subtítulo do Card Centralizado
                const char *subTitulo = "Novo Agendamento / Cadastro Rápido";
                float largSub = ImGui::CalcTextSize(subTitulo).x;
                ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - largSub) * 0.5f);
                ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive], "%s", subTitulo);
                ImGui::Spacing();

                if (ImGui::BeginTable("TabelaFormularioCadastro", 2, ImGuiTableFlags_SizingFixedFit, ImVec2(0, 0)))
                {
                    ImGui::TableSetupColumn("Rotulo", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                    ImGui::TableSetupColumn("Campo", ImGuiTableColumnFlags_WidthStretch);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Nome do Cliente:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::InputText("##inputCli", ctx.nomeCliente, IM_ARRAYSIZE(ctx.nomeCliente));

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Telefone:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::InputText("##inputTel", ctx.telefoneCliente, IM_ARRAYSIZE(ctx.telefoneCliente));

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Observações / Serviço:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::InputText("##inputObs", ctx.obsServico, IM_ARRAYSIZE(ctx.obsServico));

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Valor (R$):");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::InputFloat("##inputVal", &ctx.valorServico, 0.01f, 1.0f, "%.2f");

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Data e Horário:");
                    ImGui::TableSetColumnIndex(1);

                    // Seleção harmoniosa de Data e Hora
                    ImGui::SetNextItemWidth(270.0f);
                    int indiceAntigo = ctx.dataComboIndex;
                    if (ImGui::Combo("##dataCombo", &ctx.dataComboIndex, diasArr, 7))
                    {
                        std::string dataSelecionada = datasPuras[ctx.dataComboIndex];
                        if (dataSelecionada < dataHojeStr)
                        {
                            ctx.dataComboIndex = indiceAntigo;
                            ctx.mensagemErroConflito = "Aviso: Não é permitido selecionar dias passados!";
                        }
                        else
                        {
                            ctx.mensagemErroConflito = "";
                        }
                    }

                    ImGui::SameLine();
                    const char *horasArr[] = {"08", "09", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20"};
                    ImGui::SetNextItemWidth(60.0f);
                    ImGui::Combo("##horaCombo", &ctx.horaIndex, horasArr, IM_ARRAYSIZE(horasArr));

                    ImGui::SameLine();
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text(":");
                    ImGui::SameLine();

                    const char *minsArr[] = {"00", "30"};
                    ImGui::SetNextItemWidth(60.0f);
                    ImGui::Combo("##minCombo", &ctx.minIndex, minsArr, IM_ARRAYSIZE(minsArr));

                    ImGui::EndTable();
                }

                ImGui::Spacing();
                float larguraDisponivelCard = ImGui::GetContentRegionAvail().x;

                if (BotaoSucesso("Confirmar Agendamento##btnSalvarAgend", ImVec2(larguraDisponivelCard, 34)))
                {
                    if (strlen(ctx.nomeCliente) > 0)
                    {
                        std::string dataSelecionada = datasPuras[ctx.dataComboIndex];
                        if (dataSelecionada < dataHojeStr)
                        {
                            ctx.mensagemErroConflito = "Erro: Não é permitido agendar em datas anteriores!";
                        }
                        else
                        {
                            const char *horasArr[] = {"08", "09", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20"};
                            const char *minsArr[] = {"00", "30"};
                            std::string horaSelecionada = std::string(horasArr[ctx.horaIndex]) + ":" + std::string(minsArr[ctx.minIndex]);
                            std::string dataHoraCompleta = dataSelecionada + " " + horaSelecionada + ":00";

                            std::string sqlCheck = "SELECT COUNT(*) FROM agendamentos WHERE barbeiro_id = " + std::to_string(ctx.cadeiraSelecionadaID) + " AND data_hora = '" + dataHoraCompleta + "';";
                            sqlite3_stmt *stmtCheck = nullptr;
                            bool temConflito = false;

                            if (sqlite3_prepare_v2(db, sqlCheck.c_str(), -1, &stmtCheck, nullptr) == SQLITE_OK)
                            {
                                if (sqlite3_step(stmtCheck) == SQLITE_ROW)
                                {
                                    if (sqlite3_column_int(stmtCheck, 0) > 0)
                                        temConflito = true;
                                }
                                sqlite3_finalize(stmtCheck);
                            }

                            if (temConflito)
                            {
                                ctx.mensagemErroConflito = "Erro: Este horário já está ocupado para este barbeiro!";
                            }
                            else
                            {
                                const char *sqlInsert = "INSERT INTO agendamentos (usuario_id, barbeiro_id, cliente_nome, servico, valor, data_hora) VALUES (1, ?, ?, ?, ?, ?);";
                                sqlite3_stmt *stmt;
                                if (sqlite3_prepare_v2(db, sqlInsert, -1, &stmt, nullptr) == SQLITE_OK)
                                {
                                    sqlite3_bind_int(stmt, 1, ctx.cadeiraSelecionadaID);
                                    sqlite3_bind_text(stmt, 2, ctx.nomeCliente, -1, SQLITE_TRANSIENT);
                                    std::string servicoCompleto = std::string(ctx.obsServico) + " - Tel: " + std::string(ctx.telefoneCliente);
                                    sqlite3_bind_text(stmt, 3, servicoCompleto.c_str(), -1, SQLITE_TRANSIENT);
                                    sqlite3_bind_double(stmt, 4, ctx.valorServico);
                                    sqlite3_bind_text(stmt, 5, dataHoraCompleta.c_str(), -1, SQLITE_TRANSIENT);

                                    sqlite3_step(stmt);
                                    sqlite3_finalize(stmt);

                                    memset(ctx.nomeCliente, 0, sizeof(ctx.nomeCliente));
                                    memset(ctx.telefoneCliente, 0, sizeof(ctx.telefoneCliente));
                                    memset(ctx.obsServico, 0, sizeof(ctx.obsServico));
                                    ctx.mensagemErroConflito = "";
                                }
                            }
                        }
                    }
                    else
                    {
                        ctx.mensagemErroConflito = "Erro: O nome do cliente é obrigatório!";
                    }
                }
            }
            ImGui::EndChild();

            if (!ctx.mensagemErroConflito.empty())
            {
                ImGui::Spacing();
                float largErr = ImGui::CalcTextSize(ctx.mensagemErroConflito.c_str()).x;
                ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - largErr) * 0.5f);
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
                ImGui::TextWrapped("⚠ %s", ctx.mensagemErroConflito.c_str());
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // --- FILTROS CENTRALIZADOS ---
            float larguraJanela = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX((larguraJanela - 440.0f) * 0.5f);
            ImGui::BeginGroup();
            ImGui::Text("Filtrar:");
            ImGui::SameLine();
            ImGui::RadioButton("Apenas Hoje##f0", &ctx.filtroAgenda, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Próximos 7 Dias##f1", &ctx.filtroAgenda, 1);
            ImGui::SameLine();
            ImGui::RadioButton("Mostrar Todos##f2", &ctx.filtroAgenda, 2);
            ImGui::EndGroup();

            ImGui::Spacing();

            // --- LEGENDA DE CORES CENTRALIZADA ---
            ImGui::SetCursorPosX((larguraJanela - 390.0f) * 0.5f);
            ImVec2 tamBloco(14, 14);
            ImGui::ColorButton("##cHoje", ImVec4(0.2f, 0.9f, 0.2f, 1.0f), ImGuiColorEditFlags_NoTooltip, tamBloco);
            ImGui::SameLine();
            ImGui::Text("Hoje");
            ImGui::SameLine(0, 18);
            ImGui::ColorButton("##cProx", ImVec4(0.9f, 0.9f, 0.2f, 1.0f), ImGuiColorEditFlags_NoTooltip, tamBloco);
            ImGui::SameLine();
            ImGui::Text("1-3 dias");
            ImGui::SameLine(0, 18);
            ImGui::ColorButton("##cDist", ImVec4(0.9f, 0.3f, 0.3f, 1.0f), ImGuiColorEditFlags_NoTooltip, tamBloco);
            ImGui::SameLine();
            ImGui::Text("4d+");
            ImGui::SameLine(0, 18);
            ImGui::ColorButton("##cPass", ImVec4(0.6f, 0.6f, 0.6f, 1.0f), ImGuiColorEditFlags_NoTooltip, tamBloco);
            ImGui::SameLine();
            ImGui::Text("Passados");

            ImGui::Spacing();

            float lucroTotalCadeira = 0.0f;

            // --- TABELA DE AGENDAMENTOS ORGANIZADA E CENTRALIZADA ---
            if (ImGui::BeginTable("TabelaAgendamentos", 6, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 200)))
            {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 40);
                ImGui::TableSetupColumn("Cliente", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Telefone / Obs", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Valor", ImGuiTableColumnFlags_WidthFixed, 85);
                ImGui::TableSetupColumn("Data/Hora", ImGuiTableColumnFlags_WidthFixed, 140);
                ImGui::TableSetupColumn("Ações", ImGuiTableColumnFlags_WidthFixed, 140);
                ImGui::TableHeadersRow();

                std::string sqlAgend = "SELECT id, cliente_nome, servico, valor, data_hora FROM agendamentos WHERE barbeiro_id = " + std::to_string(ctx.cadeiraSelecionadaID);
                if (ctx.filtroAgenda == 0)
                    sqlAgend += " AND data_hora LIKE '" + dataHojeStr + "%';";
                else if (ctx.filtroAgenda == 1)
                    sqlAgend += " AND data_hora >= '" + std::string(datasPuras[0]) + "' AND data_hora <= '" + std::string(datasPuras[6]) + " 23:59:59';";
                else
                    sqlAgend += ";";

                int idParaConcluir = -1;
                int idParaExcluir = -1;
                std::string clienteConcluir = "";
                std::string servicoConcluir = "";
                double valorConcluir = 0.0;
                std::string dataHoraConcluir = "";

                sqlite3_stmt *stmtAgend = nullptr;
                if (sqlite3_prepare_v2(db, sqlAgend.c_str(), -1, &stmtAgend, nullptr) == SQLITE_OK)
                {
                    while (sqlite3_step(stmtAgend) == SQLITE_ROW)
                    {
                        int idAg = sqlite3_column_int(stmtAgend, 0);
                        const unsigned char *cli = sqlite3_column_text(stmtAgend, 1);
                        const unsigned char *srv = sqlite3_column_text(stmtAgend, 2);
                        double val = sqlite3_column_double(stmtAgend, 3);
                        const unsigned char *dh = sqlite3_column_text(stmtAgend, 4);

                        lucroTotalCadeira += (float)val;

                        std::string nomeCliRow = cli ? std::string((const char *)cli) : "";
                        std::string servSrvRow = srv ? std::string((const char *)srv) : "";
                        std::string dataStr = dh ? std::string((const char *)dh).substr(0, 10) : "";

                        ImVec4 corTexto = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
                        if (dataStr < dataHojeStr)
                            corTexto = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
                        else if (dataStr == dataHojeStr)
                            corTexto = ImVec4(0.2f, 0.9f, 0.2f, 1.0f);
                        else
                        {
                            int anoH, mesH, diaH, anoD, mesD, diaD;
                            sscanf(dataHojeStr.c_str(), "%d-%d-%d", &anoH, &mesH, &diaH);
                            sscanf(dataStr.c_str(), "%d-%d-%d", &anoD, &mesD, &diaD);
                            long diasDif = (anoD * 365 + mesD * 31 + diaD) - (anoH * 365 + mesH * 31 + diaH);

                            if (diasDif >= 1 && diasDif <= 3)
                                corTexto = ImVec4(0.9f, 0.9f, 0.2f, 1.0f);
                            else if (diasDif > 3)
                                corTexto = ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
                        }

                        ImGui::TableNextRow();
                        ImGui::PushID(idAg);

                        // ID Centralizado
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%d", idAg);

                        // Nome e Serviços
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s", nomeCliRow.c_str());

                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%s", servSrvRow.c_str());

                        // Valor R$
                        ImGui::TableSetColumnIndex(3);
                        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "R$ %.2f", val);

                        // Data/Hora
                        ImGui::TableSetColumnIndex(4);
                        ImGui::TextColored(corTexto, "%s", dh ? (const char *)dh : "");

                        // Ações compactas e alinhadas
                        ImGui::TableSetColumnIndex(5);
                        if (BotaoSucesso("Feito##btnFeito", ImVec2(60, 24)))
                        {
                            idParaConcluir = idAg;
                            clienteConcluir = nomeCliRow;
                            servicoConcluir = servSrvRow;
                            valorConcluir = val;
                            dataHoraConcluir = dh ? (const char *)dh : dataHojeStr;
                        }

                        ImGui::SameLine();

                        if (BotaoCancelar("Excluir##btnDel", ImVec2(65, 24)))
                        {
                            idParaExcluir = idAg;
                        }

                        ImGui::PopID();
                    }

                    sqlite3_finalize(stmtAgend);
                }

                ImGui::EndTable();

                // Processamento de Banco de Dados sem Crash
                if (idParaConcluir != -1)
                {
                    const char *sqlInsereHist = "INSERT INTO faturamento_historico (barbeiro_id, cliente, servico, valor, data) VALUES (?, ?, ?, ?, ?);";
                    sqlite3_stmt *stmtHistIns;
                    if (sqlite3_prepare_v2(db, sqlInsereHist, -1, &stmtHistIns, nullptr) == SQLITE_OK)
                    {
                        sqlite3_bind_int(stmtHistIns, 1, ctx.cadeiraSelecionadaID);
                        sqlite3_bind_text(stmtHistIns, 2, clienteConcluir.c_str(), -1, SQLITE_TRANSIENT);
                        sqlite3_bind_text(stmtHistIns, 3, servicoConcluir.c_str(), -1, SQLITE_TRANSIENT);
                        sqlite3_bind_double(stmtHistIns, 4, valorConcluir);
                        sqlite3_bind_text(stmtHistIns, 5, dataHoraConcluir.c_str(), -1, SQLITE_TRANSIENT);

                        sqlite3_step(stmtHistIns);
                        sqlite3_finalize(stmtHistIns);
                    }

                    std::string sqlDel = "DELETE FROM agendamentos WHERE id = " + std::to_string(idParaConcluir) + ";";
                    sqlite3_exec(db, sqlDel.c_str(), 0, 0, 0);

                    CarregarHistoricoCadeira(db, ctx.cadeiraSelecionadaID, 30);
                }

                if (idParaExcluir != -1)
                {
                    std::string sqlDelAgend = "DELETE FROM agendamentos WHERE id = " + std::to_string(idParaExcluir) + ";";
                    sqlite3_exec(db, sqlDelAgend.c_str(), 0, 0, 0);
                }

                ImGui::Spacing();

                // Totalizador Centralizado no Rodapé
                float largLucro = 320.0f;
                ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - largLucro) * 0.5f);
                ImGui::Text("Lucro Total da Cadeira no Filtro:");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "R$ %.2f", lucroTotalCadeira);
            }
        }

        ImGui::EndPopup();
    }
}

bool ChecarAgendamentoAtrasado(sqlite3 *db, int barbeiroID)
{
    // Pega o agendamento de HOJE mais antigo que já passou do horário
    const char *sql = "SELECT data_hora FROM agendamentos "
                      "WHERE barbeiro_id = ? AND date(data_hora) = date('now', 'localtime') "
                      "ORDER BY data_hora ASC LIMIT 1;";

    sqlite3_stmt *stmt = nullptr;
    bool atrasado = false;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, barbeiroID);

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const char *dataHoraStr = (const char *)sqlite3_column_text(stmt, 0);

            if (dataHoraStr)
            {
                int ano = 0, mes = 0, dia = 0, hora = 0, min = 0;

                if (sscanf(dataHoraStr, "%d-%d-%d %d:%d", &ano, &mes, &dia, &hora, &min) >= 5)
                {
                    struct tm tmAgendado = {};
                    tmAgendado.tm_year = ano - 1900;
                    tmAgendado.tm_mon = mes - 1;
                    tmAgendado.tm_mday = dia;
                    tmAgendado.tm_hour = hora;
                    tmAgendado.tm_min = min;
                    tmAgendado.tm_isdst = -1;

                    time_t tempoAgendado = mktime(&tmAgendado);
                    time_t tempoAgora = time(nullptr);

                    double diferencaSegundos = difftime(tempoAgora, tempoAgendado);

                    // Só considera ATRASADO se o horário já começou (diferenca > 0)
                    // E se ultrapassou os 10 minutos de tolerância (> 600 segundos)
                    if (diferencaSegundos > 600)
                    {
                        atrasado = true;
                    }
                }
            }
        }
        sqlite3_finalize(stmt);
    }

    return atrasado;
}

void RenderizarModalAdicionarFoto(sqlite3 *db)
{
    if (!ctx.modalAdicionarFoto)
        return;

    ImGui::OpenPopup("Alterar Foto do Barbeiro##modalFoto");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Alterar Foto do Barbeiro##modalFoto", &ctx.modalAdicionarFoto, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Informe o caminho ou selecione a imagem do barbeiro:");
        ImGui::Spacing();

        ImGui::PushItemWidth(260.0f);
        ImGui::InputText("##caminhoFotoInput", ctx.caminhoFotoInput, sizeof(ctx.caminhoFotoInput));
        ImGui::PopItemWidth();

        ImGui::SameLine();

        // Botão para abrir o seletor de arquivos do Windows
        if (ImGui::Button("Procurar...##btnArquivo"))
        {
            OPENFILENAMEA ofn;
            char szFile[260] = {0};

            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = NULL;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = "Imagens (*.png;*.jpg;*.jpeg)\0*.png;*.jpg;*.jpeg\0Todos os Arquivos (*.*)\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

            if (GetOpenFileNameA(&ofn) == TRUE)
            {
                snprintf(ctx.caminhoFotoInput, sizeof(ctx.caminhoFotoInput), "%s", ofn.lpstrFile);
            }
        }

        ImGui::TextDisabled("Exemplo: C:\\fotos\\barbeiro.png");
        ImGui::Separator();
        ImGui::Spacing();

        // Botão Salvar
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.4f, 1.0f));
        if (ImGui::Button("Salvar Foto", ImVec2(120, 30)))
        {
            std::string sqlUpdate = "UPDATE barbeiros SET foto_path = ? WHERE id = ?;";
            sqlite3_stmt *stmt = nullptr;

            if (sqlite3_prepare_v2(db, sqlUpdate.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
            {
                sqlite3_bind_text(stmt, 1, ctx.caminhoFotoInput, -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(stmt, 2, ctx.cadeiraSelecionadaID);

                if (sqlite3_step(stmt) == SQLITE_DONE)
                {
                    // Limpa do cache para forçar a nova leitura/renderização imediata na tela
                    g_cacheTexturasBarbeiros.erase(ctx.caminhoFotoInput);
                }
                sqlite3_finalize(stmt);
            }

            ctx.modalAdicionarFoto = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(2);

        ImGui::SameLine();

        // Botão Cancelar
        if (ImGui::Button("Cancelar", ImVec2(100, 30)))
        {
            ctx.modalAdicionarFoto = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}
// ==========================================
// 3. FUNÇÃO PRINCIPAL DE RENDERIZAÇÃO
// ==========================================

void CadeirasBarber()
{
    time_t agora = time(0);
    tm *ltm = localtime(&agora);
    char bufferDataHoje[11];
    sprintf(bufferDataHoje, "%04d-%02d-%02d", 1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday);
    std::string dataHojeStr(bufferDataHoje);

    // --- BUSCAR BARBEIROS ---
    struct BarbeiroInfoLocal
    {
        int id;
        std::string nome;
        std::string fotoPath;
    };

    std::vector<BarbeiroInfoLocal> listaBarbeiros;
    const char *sqlSelectBarbeiros = "SELECT id, nome, foto_path FROM barbeiros;";
    sqlite3_stmt *stmtBarbeiros = nullptr;
    if (sqlite3_prepare_v2(db, sqlSelectBarbeiros, -1, &stmtBarbeiros, nullptr) == SQLITE_OK)
    {
        while (sqlite3_step(stmtBarbeiros) == SQLITE_ROW)
        {
            int id = sqlite3_column_int(stmtBarbeiros, 0);
            const unsigned char *nome = sqlite3_column_text(stmtBarbeiros, 1);
            const unsigned char *foto = sqlite3_column_text(stmtBarbeiros, 2);

            listaBarbeiros.push_back({id,
                                      nome ? std::string((const char *)nome) : "",
                                      foto ? std::string((const char *)foto) : ""});
        }
        sqlite3_finalize(stmtBarbeiros);
    }

    // --- BARRA SUPERIOR ---
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.4f, 1.0f));

    if (ImGui::Button("+ Adicionar Cadeira##btnMainAdd", ImVec2(160, 30)))
    {
        FecharTodasAsJanelasExclusivas();
        ctx.modalAdicionar = true;
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));

    if (ImGui::Button("Configurar Pix##btnConfigPix", ImVec2(130, 30)))
    {
        FecharTodasAsJanelasExclusivas();
        ctx.modalConfigPix = true;
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.50f, 0.10f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.60f, 0.20f, 1.0f));

    if (ImGui::Button("📱 Ver / Copiar Pix##btnVerPix", ImVec2(150, 30)))
    {
        FecharTodasAsJanelasExclusivas();
        ctx.modalReceberPix = true;
    }
    ImGui::PopStyleColor(2);

    ImGui::Separator();
    ImGui::Spacing();

    // --- GRID DE CADEIRAS ---
    if (listaBarbeiros.empty())
    {
        ImGui::TextDisabled("Nenhuma cadeira/barbeiro cadastrada. Clique em '+ Adicionar Cadeira'.");
    }
    else
    {
        int totalColunas = std::max(1, (int)listaBarbeiros.size());
        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_RowBg;

        if (ImGui::BeginTable("TabelaCadeirasBarbearia", totalColunas, flags))
        {
            // Aumentada a altura da linha da tabela para acomodar o card vertical estilo smartphone (400px)
            ImGui::TableNextRow(ImGuiTableRowFlags_None, 350.0f);

            for (size_t i = 0; i < listaBarbeiros.size(); i++)
            {
                int barbeiroID = listaBarbeiros[i].id;
                ImGui::TableNextColumn();
                ImGui::PushID(barbeiroID);

                // 1. LER O STATUS ATUAL DO BARBEIRO ANTES DE DEFINIR OS ESTILOS
                std::string statusAtualStr = "Livre";
                std::string sqlStatus = "SELECT status_atendimento FROM barbeiros WHERE id = " + std::to_string(barbeiroID) + ";";
                sqlite3_stmt *stmtStatus;
                if (sqlite3_prepare_v2(db, sqlStatus.c_str(), -1, &stmtStatus, nullptr) == SQLITE_OK)
                {
                    if (sqlite3_step(stmtStatus) == SQLITE_ROW)
                    {
                        const unsigned char *val = sqlite3_column_text(stmtStatus, 0);
                        if (val)
                            statusAtualStr = reinterpret_cast<const char *>(val);
                    }
                    sqlite3_finalize(stmtStatus);
                }

                bool temAtrasado = ChecarAgendamentoAtrasado(db, barbeiroID);
                bool temAtendimentoIniciado = false;
                std::string sqlTolerancia = "SELECT 1 FROM agendamentos WHERE barbeiro_id = ? "
                                            "AND datetime(data_hora) <= datetime('now', 'localtime') "
                                            "AND datetime(data_hora) >= datetime('now', 'localtime', '-10 minutes') LIMIT 1;";
                sqlite3_stmt *stmtTol = nullptr;
                if (sqlite3_prepare_v2(db, sqlTolerancia.c_str(), -1, &stmtTol, nullptr) == SQLITE_OK)
                {
                    sqlite3_bind_int(stmtTol, 1, barbeiroID);
                    if (sqlite3_step(stmtTol) == SQLITE_ROW)
                    {
                        temAtendimentoIniciado = true;
                    }
                    sqlite3_finalize(stmtTol);
                }

                bool estaAtrasado = temAtrasado || (statusAtualStr == "Atrasado" || statusAtualStr == "atrasado");
                bool estaEmAtendimento = !estaAtrasado && (temAtendimentoIniciado || statusAtualStr == "Ocupado" || statusAtualStr == "Em Atendimento");
                bool estaLivre = !estaAtrasado && !estaEmAtendimento && (statusAtualStr == "Livre" || statusAtualStr == "livre");

                // 2. DEFINIR A COR DA BORDA COM BASE NO STATUS
                ImVec4 corBordaDinamica;
                if (estaAtrasado)
                {
                    corBordaDinamica = ImVec4(1.0f, 0.25f, 0.25f, 0.95f); // Vermelho Alerta
                }
                else if (estaEmAtendimento)
                {
                    corBordaDinamica = ImVec4(0.95f, 0.70f, 0.15f, 0.95f); // Amarelo/Dourado Atendimento
                }
                else
                {
                    corBordaDinamica = ImVec4(0.20f, 0.85f, 0.40f, 0.95f); // Verde Livre
                }

                // 3. APLICAR OS ESTILOS DA BORDA E DO CONTAINER
                ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.5f);
                ImGui::PushStyleColor(ImGuiCol_Border, corBordaDinamica);
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.07f, 0.10f, 0.95f));

                std::string idContainerCadeira = "container_cadeira_" + std::to_string(barbeiroID);
                if (ImGui::BeginChild(idContainerCadeira.c_str(), ImVec2(0, 350.0f), true, ImGuiWindowFlags_NoScrollbar))
                {
                    // Renderizar foto no fundo da cadeira (se houver)
                    if (!listaBarbeiros[i].fotoPath.empty())
                    {
                        GLuint texID = ObterOuCarregarTextura(listaBarbeiros[i].fotoPath);
                        if (texID != 0)
                        {
                            ImVec2 minPos = ImGui::GetCursorScreenPos();
                            float larguraCard = ImGui::GetContentRegionAvail().x;
                            float alturaCard = 350.0f;

                            ImVec2 maxPos = ImVec2(minPos.x + larguraCard, minPos.y + alturaCard);
                            ImU32 corFotoDestaque = IM_COL32(255, 255, 255, 120);

                            ImGui::GetWindowDrawList()->AddImage(
                                (void *)(intptr_t)texID,
                                minPos,
                                maxPos,
                                ImVec2(0, 0),
                                ImVec2(1, 1),
                                corFotoDestaque);
                        }
                    }

                    // 2. Ler o status atual
                    std::string statusAtualStr = "Livre";
                    std::string sqlStatus = "SELECT status_atendimento FROM barbeiros WHERE id = " + std::to_string(barbeiroID) + ";";
                    sqlite3_stmt *stmtStatus;
                    if (sqlite3_prepare_v2(db, sqlStatus.c_str(), -1, &stmtStatus, nullptr) == SQLITE_OK)
                    {
                        if (sqlite3_step(stmtStatus) == SQLITE_ROW)
                        {
                            const unsigned char *val = sqlite3_column_text(stmtStatus, 0);
                            if (val)
                                statusAtualStr = reinterpret_cast<const char *>(val);
                        }
                        sqlite3_finalize(stmtStatus);
                    }

                    bool temAtrasado = ChecarAgendamentoAtrasado(db, barbeiroID);
                    bool temAtendimentoIniciado = false;
                    std::string sqlTolerancia = "SELECT 1 FROM agendamentos WHERE barbeiro_id = ? "
                                                "AND datetime(data_hora) <= datetime('now', 'localtime') "
                                                "AND datetime(data_hora) >= datetime('now', 'localtime', '-10 minutes') LIMIT 1;";
                    sqlite3_stmt *stmtTol = nullptr;
                    if (sqlite3_prepare_v2(db, sqlTolerancia.c_str(), -1, &stmtTol, nullptr) == SQLITE_OK)
                    {
                        sqlite3_bind_int(stmtTol, 1, barbeiroID);
                        if (sqlite3_step(stmtTol) == SQLITE_ROW)
                        {
                            temAtendimentoIniciado = true;
                        }
                        sqlite3_finalize(stmtTol);
                    }

                    bool estaAtrasado = temAtrasado || (statusAtualStr == "Atrasado" || statusAtualStr == "atrasado");
                    bool estaEmAtendimento = !estaAtrasado && (temAtendimentoIniciado || statusAtualStr == "Ocupado" || statusAtualStr == "Em Atendimento");
                    bool estaLivre = !estaAtrasado && !estaEmAtendimento && (statusAtualStr == "Livre" || statusAtualStr == "livre");

                    std::string proximoCliente = buscarProximoAgendamento(db, barbeiroID, dataHojeStr);

                    // ==========================================
                    // CONTEÚDO VISUAL INTERNO DA CADEIRA
                    // ==========================================

                    // A. Título da Cadeira Centralizado
                    char tituloCadeira[32];
                    snprintf(tituloCadeira, sizeof(tituloCadeira), "CADEIRA %d", static_cast<int>(i + 1));
                    float larguraColuna = ImGui::GetContentRegionAvail().x;
                    float larguraTextoCadeira = ImGui::CalcTextSize(tituloCadeira).x;

                    float startXCadeira = (larguraColuna > larguraTextoCadeira) ? (larguraColuna - larguraTextoCadeira) * 0.5f : 0.0f;
                    ImGui::SetCursorPosX(startXCadeira);
                    ImGui::Text("%s", tituloCadeira);
                    ImGui::Separator();

                    ImGui::Spacing();

                    // B. Card do Barbeiro Centralizado
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.08f, 0.85f));
                    std::string idCardBarbeiro = "card_barbeiro_" + std::to_string(barbeiroID);
                    if (ImGui::BeginChild(idCardBarbeiro.c_str(), ImVec2(0, 42), true, ImGuiWindowFlags_NoScrollbar))
                    {
                        const char *labelBarbeiro = "Barbeiro: ";
                        const char *nomeBarbeiro = listaBarbeiros[i].nome.c_str();

                        float labelWidth = ImGui::CalcTextSize(labelBarbeiro).x;
                        float nameWidth = ImGui::CalcTextSize(nomeBarbeiro).x;
                        float totalWidthBarbeiro = labelWidth + nameWidth;
                        float availWidthBarbeiro = ImGui::GetContentRegionAvail().x;

                        float startX = (availWidthBarbeiro > totalWidthBarbeiro) ? (availWidthBarbeiro - totalWidthBarbeiro) * 0.5f : 0.0f;
                        ImGui::SetCursorPosX(startX);
                        ImGui::SetCursorPosY((42.0f - ImGui::GetTextLineHeight()) * 0.5f);

                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", labelBarbeiro);
                        ImGui::SameLine(0, 0);
                        ImGui::TextColored(ImVec4(0.3f, 0.85f, 1.0f, 1.0f), "%s", nomeBarbeiro);
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleColor();

                    ImGui::Spacing();

                    // C. Rótulo "Próximo Agendamento:" Centralizado e Destacado em Amarelo
                    const char *tituloProx = "Próximo Agendamento:";
                    float tituloWidth = ImGui::CalcTextSize(tituloProx).x;
                    float availWidthWindow = ImGui::GetContentRegionAvail().x;
                    float startXTitulo = (availWidthWindow > tituloWidth) ? (availWidthWindow - tituloWidth) * 0.5f : 0.0f;

                    ImGui::SetCursorPosX(startXTitulo);
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", tituloProx);

                    ImGui::Spacing();

                    // 4. Card do Próximo Cliente (Horário + Nome com Destaque)
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.08f, 0.10f, 0.95f));

                    std::string idCardProx = "card_prox_" + std::to_string(barbeiroID);
                    if (ImGui::BeginChild(idCardProx.c_str(), ImVec2(0, 48), true, ImGuiWindowFlags_NoScrollbar))
                    {
                        float availWidthProx = ImGui::GetContentRegionAvail().x;

                        if (proximoCliente.empty() || proximoCliente == "Nenhum agendamento próximo")
                        {
                            const char *textNenhum = "Nenhum agendamento próximo";
                            float textWidth = ImGui::CalcTextSize(textNenhum).x;
                            float startX = (availWidthProx > textWidth) ? (availWidthProx - textWidth) * 0.5f : 0.0f;

                            ImGui::SetCursorPosX(startX);
                            ImGui::SetCursorPosY((48.0f - ImGui::GetTextLineHeight()) * 0.5f);
                            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", textNenhum);
                        }
                        else
                        {
                            // Separa a hora ("13:00 - ") do nome ("teste dias") para dar cores diferentes
                            size_t posHifen = proximoCliente.find(" - ");
                            if (posHifen != std::string::npos)
                            {
                                std::string horaStr = proximoCliente.substr(0, posHifen + 3); // Extrai "HH:MM - "
                                std::string nomeStr = proximoCliente.substr(posHifen + 3);    // Extrai o nome do cliente

                                float widthHora = ImGui::CalcTextSize(horaStr.c_str()).x;
                                float widthNome = ImGui::CalcTextSize(nomeStr.c_str()).x;
                                float totalWidth = widthHora + widthNome;

                                float startX = (availWidthProx > totalWidth) ? (availWidthProx - totalWidth) * 0.5f : 0.0f;

                                ImGui::SetCursorPosX(startX);
                                ImGui::SetCursorPosY((48.0f - ImGui::GetTextLineHeight()) * 0.5f);

                                // Horário em Amarelo/Dourado de alto destaque
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.30f, 1.0f), "%s", horaStr.c_str());
                                ImGui::SameLine(0, 0);
                                // Nome do cliente em Branco
                                ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", nomeStr.c_str());
                            }
                            else
                            {
                                // Fallback para exibição padrão caso a string venha em formato diferente
                                float textWidth = ImGui::CalcTextSize(proximoCliente.c_str()).x;
                                float startX = (availWidthProx > textWidth) ? (availWidthProx - textWidth) * 0.5f : 0.0f;

                                ImGui::SetCursorPosX(startX);
                                ImGui::SetCursorPosY((48.0f - ImGui::GetTextLineHeight()) * 0.5f);
                                ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.95f, 1.00f), "%s", proximoCliente.c_str());
                            }
                        }
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleColor();

                    ImGui::Spacing();

                    // E. Status Visual Centralizado e Destacado
                    const char *textoStatus = estaAtrasado ? "● ATRASADO!" : (estaEmAtendimento ? "● Em Atendimento" : "● Livre");
                    ImVec4 corStatus = estaAtrasado ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) : (estaEmAtendimento ? ImVec4(0.9f, 0.7f, 0.1f, 1.0f) : ImVec4(0.2f, 0.9f, 0.4f, 1.0f));

                    float larguraTextoStatus = ImGui::CalcTextSize(textoStatus).x;
                    float startXStatus = (larguraColuna > larguraTextoStatus) ? (larguraColuna - larguraTextoStatus) * 0.5f : 0.0f;
                    ImGui::SetCursorPosX(startXStatus);
                    ImGui::TextColored(corStatus, "%s", textoStatus);

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    // F. Botões de Ação Principais
                    if (!estaLivre)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.60f, 0.10f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.70f, 0.20f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.50f, 0.00f, 1.0f));

                        if (ImGui::Button("🧾 Fechar Comanda / Pagamento##btnComandaPadrao", ImVec2(-1.0f, 35.0f)))
                        {
                            FecharTodasAsJanelasExclusivas();
                            ctx.cadeiraSelecionadaID = barbeiroID;
                            ctx.modalFechamento = true;
                        }
                        ImGui::PopStyleColor(3);
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.3f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.4f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.2f, 1.0f));

                        if (ImGui::Button("Registrar Encaixe##btnStatus", ImVec2(-1.0f, 30.0f)))
                        {
                            FecharTodasAsJanelasExclusivas();
                            ctx.cadeiraSelecionadaParaEncaixe = barbeiroID;
                            ctx.modalEncaixe = true;
                        }
                        ImGui::PopStyleColor(3);
                    }

                    ImGui::Spacing();
                    if (ImGui::Button("Gerenciar / Agendar", ImVec2(-1.0f, 32.0f)))
                    {
                        FecharTodasAsJanelasExclusivas();

                        ctx.cadeiraSelecionadaID = barbeiroID;
                        memset(ctx.nomeCliente, 0, sizeof(ctx.nomeCliente));
                        memset(ctx.telefoneCliente, 0, sizeof(ctx.telefoneCliente));
                        memset(ctx.obsServico, 0, sizeof(ctx.obsServico));
                        ctx.mensagemErroConflito.clear();
                        ctx.mostrandoHistorico = false;
                        ctx.modalGerenciar = true;
                    }

                    ImGui::Spacing();

                    // G. Botões Secundários Lado a Lado (Foto, Editar e Excluir)
                    float larguraDisponivel = ImGui::GetContentRegionAvail().x;
                    float espacamentoItem = ImGui::GetStyle().ItemSpacing.x;
                    float larguraBotaoTerca = (larguraDisponivel - (espacamentoItem * 2.0f)) / 3.0f;

                    // --- BOTÃO FOTO ---
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));
                    if (ImGui::Button("📷 Foto", ImVec2(larguraBotaoTerca, 28.0f)))
                    {
                        FecharTodasAsJanelasExclusivas();

                        ctx.cadeiraSelecionadaID = barbeiroID;
                        memset(ctx.caminhoFotoInput, 0, sizeof(ctx.caminhoFotoInput));
                        ctx.modalAdicionarFoto = true;
                    }
                    ImGui::PopStyleColor(2);

                    ImGui::SameLine();

                    // --- BOTÃO EDITAR ---
                    if (ImGui::Button("Editar", ImVec2(larguraBotaoTerca, 28.0f)))
                    {
                        FecharTodasAsJanelasExclusivas();

                        ctx.cadeiraSelecionadaID = barbeiroID;
                        snprintf(ctx.editNomeBarbeiro, sizeof(ctx.editNomeBarbeiro), "%s", listaBarbeiros[i].nome.c_str());
                        ctx.modalEditar = true;
                    }

                    ImGui::SameLine();

                    // --- BOTÃO EXCLUIR ---
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
                    if (ImGui::Button("Excluir", ImVec2(larguraBotaoTerca, 28.0f)))
                    {
                        FecharTodasAsJanelasExclusivas();

                        ctx.cadeiraSelecionadaID = barbeiroID;
                        memset(ctx.senhaExclusao, 0, sizeof(ctx.senhaExclusao));
                        ctx.erroSenhaExclusao.clear();
                        ctx.modalExcluir = true;
                    }
                    ImGui::PopStyleColor(2);
                }
                ImGui::EndChild(); // Fecha o container principal da cadeira
                ImGui::PopStyleColor(2);
                ImGui::PopStyleVar();

                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }

    // RENDERIZADORES DE MODAIS
    RenderizarModalAdicionarCadeira(db);
    RenderizarModalEditarCadeira(db);
    RenderizarModalEncaixeRapido(db);
    RenderizarModalGerenciarCadeira(db);
    RenderizarModalExcluirCadeira(db);
    RenderizarModalAlertaAtraso();
    RenderizarModalConfigPix(db);
    RenderizarModalAdicionarFoto(db);
}

// ==========================================
// FUNÇÕES AUXILIARES PARA PIX //
// ==========================================

void InicializarDadosPix(sqlite3 *db)
{
    carregarConfiguracaoPix(db, ctx.usuarioLogadoId,
                            ctx.chavePixMain, sizeof(ctx.chavePixMain),
                            ctx.nomeTitularPix, sizeof(ctx.nomeTitularPix),
                            ctx.caminhoQrCode, sizeof(ctx.caminhoQrCode));
}

void RenderizarModalConfigPix(sqlite3 *db)
{
    if (ctx.modalConfigPix)
    {
        ImGui::OpenPopup("Configurar Pagamento Pix##ModalPix");
    }

    if (ImGui::BeginPopupModal("Configurar Pagamento Pix##ModalPix", &ctx.modalConfigPix, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Cadastre a Chave Pix Principal do Estabelecimento:");
        ImGui::Separator();
        ImGui::Spacing();

        // Campos de Texto principais
        ImGui::InputText("Chave Pix (CPF/CNPJ/Email/Tel)", ctx.chavePixMain, sizeof(ctx.chavePixMain));
        ImGui::InputText("Nome do Titular/Empresa", ctx.nomeTitularPix, sizeof(ctx.nomeTitularPix));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Imagem do QR Code da Barbearia:");
        ImGui::Spacing();

        // Campo editável do caminho (sem a flag ReadOnly para permitir colar ou apagar manualmente)
        ImGui::InputText("##caminhoQrCode", ctx.caminhoQrCode, sizeof(ctx.caminhoQrCode));

        ImGui::SameLine();
        if (ImGui::Button("Buscar..."))
        {
            SelecionarImagemQrCode(ctx.caminhoQrCode, sizeof(ctx.caminhoQrCode));
        }

        ImGui::SameLine();
        // Botão para limpar/apagar o caminho da imagem facilmente
        if (ImGui::Button("Apagar"))
        {
            ctx.caminhoQrCode[0] = '\0';
        }

        // Exibe o caminho atual selecionado ou aviso
        if (strlen(ctx.caminhoQrCode) > 0)
        {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Arquivo selecionado: %s", ctx.caminhoQrCode);
        }
        else
        {
            ImGui::TextDisabled("Nenhuma imagem selecionada.");
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Nota: Todos os pagamentos via QR Code das cadeiras\nserão direcionados para esta chave e imagem.");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Salvar Configurações", ImVec2(160, 30)))
        {
            // 1. Grava no banco de dados SQLite
            salvarConfiguracaoPix(db, ctx.usuarioLogadoId, ctx.chavePixMain, ctx.nomeTitularPix, ctx.caminhoQrCode);

            // 2. Recarrega na memóriaRAM para garantir sincronia
            carregarConfiguracaoPix(db, ctx.usuarioLogadoId,
                                    ctx.chavePixMain, sizeof(ctx.chavePixMain),
                                    ctx.nomeTitularPix, sizeof(ctx.nomeTitularPix),
                                    ctx.caminhoQrCode, sizeof(ctx.caminhoQrCode));

            ctx.modalConfigPix = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancelar", ImVec2(100, 30)))
        {
            // Descarta alterações não salvas
            carregarConfiguracaoPix(db, ctx.usuarioLogadoId,
                                    ctx.chavePixMain, sizeof(ctx.chavePixMain),
                                    ctx.nomeTitularPix, sizeof(ctx.nomeTitularPix),
                                    ctx.caminhoQrCode, sizeof(ctx.caminhoQrCode));

            ctx.modalConfigPix = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void RenderizarModalReceberPix()
{
    if (!ctx.modalReceberPix)
        return;

    // 1. Centraliza e fixa o tamanho
    ImGui::SetNextWindowSize(ImVec2(440, 520), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - 220.0f,
                                   ImGui::GetIO().DisplaySize.y * 0.5f - 260.0f),
                            ImGuiCond_Appearing);

    // 2. Desenha uma camada escura no fundo de toda a tela (Overlay Modal manual)
    ImDrawList *draw_list = ImGui::GetBackgroundDrawList();
    draw_list->AddRectFilled(ImVec2(0, 0), ImGui::GetIO().DisplaySize, IM_COL32(0, 0, 0, 150));

    // 3. Renderiza a janela sem a flag _NoDocking (usando apenas NoCollapse)
    if (ImGui::Begin("Cobrança e Dados Pix##ModalReceberPix", &ctx.modalReceberPix, ImGuiWindowFlags_NoCollapse))
    {
        // Garante que a janela fique em destaque sobre o fundo
        ImGui::SetWindowFocus();

        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "DADOS PARA PAGAMENTO VIA PIX");
        ImGui::Separator();
        ImGui::Spacing();

        if (strlen(ctx.chavePixMain) > 0)
        {
            ImGui::Text("Chave Pix Cadastrada:");
            ImGui::InputText("##chaveExibicao", ctx.chavePixMain, sizeof(ctx.chavePixMain), ImGuiInputTextFlags_ReadOnly);

            if (strlen(ctx.nomeTitularPix) > 0)
            {
                ImGui::TextDisabled("Titular: %s", ctx.nomeTitularPix);
            }

            ImGui::Spacing();

            // Botão 1: Copiar Chave
            if (ImGui::Button("📋 Copiar Chave Pix", ImVec2(180, 30)))
            {
                ImGui::SetClipboardText(ctx.chavePixMain);
            }

            ImGui::SameLine();

            // Botão 2: Enviar no WhatsApp
            if (ImGui::Button("💬 Enviar no WhatsApp", ImVec2(180, 30)))
            {
                std::string textoMensagem = "💈 *";
                textoMensagem += (strlen(ctx.nomeTitularPix) > 0 ? ctx.nomeTitularPix : "Barbearia");
                textoMensagem += "* 💈\n";
                textoMensagem += "___________________________________\n\n";
                textoMensagem += "Olá! Seguem os dados oficiais para pagamento via *Pix*:\n\n";
                textoMensagem += "🔑 *Chave Pix:* " + std::string(ctx.chavePixMain) + "\n";

                if (strlen(ctx.nomeTitularPix) > 0)
                {
                    textoMensagem += "👤 *Favorecido:* " + std::string(ctx.nomeTitularPix) + "\n";
                }

                textoMensagem += "\n📌 *Instruções:*\n";
                textoMensagem += "1. Copie a chave acima e utilize a opção *Pix Copia e Cola* no seu banco.\n";
                textoMensagem += "\n___________________________________\n";
                textoMensagem += "Agradecemos a preferência! ✂️✨";

                std::string textoEncoded = URLEncode(textoMensagem);
                std::string url = "https://api.whatsapp.com/send?text=" + textoEncoded;

                ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // --- ÁREA DO QR CODE ---
            if (strlen(ctx.caminhoQrCode) > 0)
            {
                if (g_TextureQrCode == 0 && g_CaminhoAtualCarregado != ctx.caminhoQrCode)
                {
                    int width = 0, height = 0;
                    g_TextureQrCode = CarregarTexturaImagem(ctx.caminhoQrCode, &width, &height);
                    g_CaminhoAtualCarregado = ctx.caminhoQrCode;
                }

                if (g_TextureQrCode != 0)
                {
                    ImGui::TextColored(ImVec4(0.3f, 0.7f, 0.9f, 1.0f), "[ QR CODE DA BARBEARIA ]");
                    ImGui::Spacing();

                    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 200.0f) * 0.5f);
                    ImGui::Image((void *)(intptr_t)g_TextureQrCode, ImVec2(200.0f, 200.0f));

                    ImGui::Spacing();
                    ImGui::TextDisabled("Apresente a tela ao cliente no balcão para leitura.");
                }
                else
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Aviso: Imagem do QR Code não pôde ser carregada.");
                }
            }
            else
            {
                ImGui::TextDisabled("Nenhum arquivo de QR Code selecionado.");
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Atenção: Nenhuma Chave Pix cadastrada!");
            ImGui::Text("Cadastre uma chave no botão 'Configurar Pix'.");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Fechar", ImVec2(120, 30)))
        {
            ctx.modalReceberPix = false;
        }
    }
    ImGui::End();
}

void RenderizarModalFechamento(sqlite3 *db)
{
    if (!ctx.modalFechamento)
        return;

    // Obtém/Cria a lista de itens específica da cadeira selecionada
    auto &comandaDaCadeira = g_ComandasPorCadeira[ctx.cadeiraSelecionadaID];

    // Dim Overlay no fundo
    ImGui::SetNextWindowSize(ImVec2(540, 680), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - 270.0f,
                                   ImGui::GetIO().DisplaySize.y * 0.5f - 340.0f),
                            ImGuiCond_Appearing);

    ImDrawList *draw_list = ImGui::GetBackgroundDrawList();
    draw_list->AddRectFilled(ImVec2(0, 0), ImGui::GetIO().DisplaySize, IM_COL32(0, 0, 0, 160));

    static float valorServico = 0.0f;
    static int ultimoBarbeiroCarregado = -1;
    static int selectedProdIdx = 0;
    static int qtdAdicionar = 1;
    static bool mostrarAvisoEstoqueComanda = false;
    static std::string msgErroEstoqueComanda = "";

    // Passamos &ctx.modalFechamento. Se o usuário clicar no 'X', vira false
    if (ImGui::Begin("Fechamento de Comanda e Pagamento##ModalFechamento", &ctx.modalFechamento, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::SetWindowFocus();

        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.3f, 1.0f), "🧾 FECHAMENTO DE COMANDA - CADEIRA %d", ctx.cadeiraSelecionadaID);
        ImGui::Separator();
        ImGui::Spacing();

        // --- 1. VALOR DO SERVIÇO PRINCIPAL ---
        if (ultimoBarbeiroCarregado != ctx.cadeiraSelecionadaID)
        {
            bool encontrouValor = false;

            std::string sqlValor = "SELECT valor FROM agendamentos WHERE barbeiro_id = ? "
                                   "ORDER BY id DESC LIMIT 1;";

            sqlite3_stmt *stmtVal = nullptr;
            if (sqlite3_prepare_v2(db, sqlValor.c_str(), -1, &stmtVal, nullptr) == SQLITE_OK)
            {
                sqlite3_bind_int(stmtVal, 1, ctx.cadeiraSelecionadaID);
                if (sqlite3_step(stmtVal) == SQLITE_ROW)
                {
                    valorServico = (float)sqlite3_column_double(stmtVal, 0);
                    encontrouValor = true;
                }
                sqlite3_finalize(stmtVal);
            }

            if (!encontrouValor)
            {
                valorServico = 20.0f;
            }

            ultimoBarbeiroCarregado = ctx.cadeiraSelecionadaID;
        }

        ImGui::Text("Serviço Principal:");
        ImGui::InputFloat("Valor Serviço (R$)##valServico", &valorServico, 5.0f, 10.0f, "%.2f");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(0.3f, 0.7f, 0.9f, 1.0f), "📦 Adicionar Consumo à Comanda:");

        // --- 2. SELEÇÃO E LANÇAMENTO DE PRODUTOS DA COMANDA ---
        struct ItemEstoqueLocal
        {
            int id;
            std::string nome;
            float preco;
            int qtdEstoque;
            std::string tipo;
        };

        std::vector<ItemEstoqueLocal> listaProdutos;
        sqlite3 *dbAtivo = (db != nullptr) ? db : ::db;

        if (dbAtivo)
        {
            const char *sqlEstoque = "SELECT id, nome, venda, estoque, tipo FROM estoque_barbearia WHERE (ativo = 1 OR ativo IS NULL) ORDER BY nome ASC;";
            sqlite3_stmt *stmtEst = nullptr;

            if (sqlite3_prepare_v2(dbAtivo, sqlEstoque, -1, &stmtEst, nullptr) == SQLITE_OK)
            {
                while (sqlite3_step(stmtEst) == SQLITE_ROW)
                {
                    ItemEstoqueLocal item;
                    item.id = sqlite3_column_int(stmtEst, 0);

                    const unsigned char *n = sqlite3_column_text(stmtEst, 1);
                    item.nome = n ? (const char *)n : "";

                    item.preco = (float)sqlite3_column_double(stmtEst, 2);
                    item.qtdEstoque = sqlite3_column_int(stmtEst, 3);

                    const unsigned char *t = sqlite3_column_text(stmtEst, 4);
                    item.tipo = t ? (const char *)t : "";

                    if (!item.nome.empty())
                    {
                        listaProdutos.push_back(item);
                    }
                }
                sqlite3_finalize(stmtEst);
            }
        }

        if (selectedProdIdx >= (int)listaProdutos.size())
            selectedProdIdx = 0;

        ImGui::Text("Selecione o Produto abaixo:");

        if (ImGui::BeginListBox("##ListaProdutosComanda", ImVec2(480, 100)))
        {
            for (int i = 0; i < (int)listaProdutos.size(); i++)
            {
                const bool is_selected = (selectedProdIdx == i);

                char itemBuf[160];
                if (listaProdutos[i].tipo != "Servico" && listaProdutos[i].qtdEstoque <= 0)
                {
                    snprintf(itemBuf, sizeof(itemBuf), "%s  (R$ %.2f  -  [ESGOTADO])",
                             listaProdutos[i].nome.c_str(),
                             listaProdutos[i].preco);
                }
                else if (listaProdutos[i].tipo == "Servico")
                {
                    snprintf(itemBuf, sizeof(itemBuf), "%s  (R$ %.2f  -  Serviço)",
                             listaProdutos[i].nome.c_str(),
                             listaProdutos[i].preco);
                }
                else
                {
                    snprintf(itemBuf, sizeof(itemBuf), "%s  (R$ %.2f  -  Est: %d)",
                             listaProdutos[i].nome.c_str(),
                             listaProdutos[i].preco,
                             listaProdutos[i].qtdEstoque);
                }

                if (ImGui::Selectable(itemBuf, is_selected))
                {
                    selectedProdIdx = i;
                }

                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndListBox();
        }

        ImGui::Spacing();
        ImGui::Text("Quantidade:");
        ImGui::SetNextItemWidth(100.0f);
        ImGui::InputInt("##qtdProdInput", &qtdAdicionar);
        if (qtdAdicionar < 1)
            qtdAdicionar = 1;

        ImGui::SameLine();

        // VERIFICAÇÃO SE O ITEM SELECIONADO ATUALMENTE TEM ESTOQUE
        bool itemSemEstoque = false;
        if (!listaProdutos.empty() && selectedProdIdx < (int)listaProdutos.size())
        {
            if (listaProdutos[selectedProdIdx].tipo != "Servico" && listaProdutos[selectedProdIdx].qtdEstoque <= 0)
            {
                itemSemEstoque = true;
            }
        }

        if (itemSemEstoque)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("+ Lançar na Comanda", ImVec2(170, 30)))
        {
            if (!listaProdutos.empty() && selectedProdIdx < (int)listaProdutos.size())
            {
                ItemEstoqueLocal prodSel = listaProdutos[selectedProdIdx];

                // Descobre quanto desse item já foi colocado na comanda atual da cadeira
                int qtdJaNaComanda = 0;
                for (const auto &item : comandaDaCadeira)
                {
                    if (item.produto_id == prodSel.id)
                    {
                        qtdJaNaComanda += item.quantidade;
                    }
                }

                // VALIDAÇÃO DE ESTOQUE DISPONÍVEL
                if (prodSel.tipo != "Servico" && (qtdJaNaComanda + qtdAdicionar) > prodSel.qtdEstoque)
                {
                    msgErroEstoqueComanda = "Estoque insuficiente para '" + prodSel.nome + "'.\n" +
                                            "Disponível em estoque: " + std::to_string(prodSel.qtdEstoque) + " un.\n" +
                                            "Já lançados na comanda: " + std::to_string(qtdJaNaComanda) + " un.";
                    mostrarAvisoEstoqueComanda = true;
                }
                else
                {
                    bool encontrado = false;
                    for (auto &item : comandaDaCadeira)
                    {
                        if (item.produto_id == prodSel.id)
                        {
                            item.quantidade += qtdAdicionar;
                            item.subtotal = item.quantidade * item.preco_unitario;
                            encontrado = true;
                            break;
                        }
                    }

                    if (!encontrado)
                    {
                        ItemComandaLancado novo;
                        novo.produto_id = prodSel.id;
                        novo.nome = prodSel.nome;
                        novo.preco_unitario = prodSel.preco;
                        novo.quantidade = qtdAdicionar;
                        novo.subtotal = qtdAdicionar * prodSel.preco;
                        comandaDaCadeira.push_back(novo);
                    }

                    qtdAdicionar = 1;
                }
            }
        }

        if (itemSemEstoque)
        {
            ImGui::EndDisabled();
        }

        // --- 3. TABELA DE HISTÓRICO DE CONSUMO ---
        ImGui::Spacing();
        ImGui::TextDisabled("Itens Consumidos nesta Comanda:");

        float totalProdutosConsumidos = 0.0f;
        if (ImGui::BeginTable("TabelaComandaItens", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg, ImVec2(0, 120)))
        {
            ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Qtd", ImGuiTableColumnFlags_WidthFixed, 40.0f);
            ImGui::TableSetupColumn("Unit.", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableSetupColumn("Subtotal", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Ação", ImGuiTableColumnFlags_WidthFixed, 45.0f);
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < comandaDaCadeira.size(); i++)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", comandaDaCadeira[i].nome.c_str());

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%d", comandaDaCadeira[i].quantidade);

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("R$ %.2f", comandaDaCadeira[i].preco_unitario);

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("R$ %.2f", comandaDaCadeira[i].subtotal);

                ImGui::TableSetColumnIndex(4);
                ImGui::PushID(static_cast<int>(i));
                if (ImGui::Button("X", ImVec2(35, 20)))
                {
                    comandaDaCadeira.erase(comandaDaCadeira.begin() + i);
                    ImGui::PopID();
                    break;
                }
                ImGui::PopID();

                totalProdutosConsumidos += comandaDaCadeira[i].subtotal;
            }
            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // --- 4. FORMA DE PAGAMENTO E CÁLCULO DINÂMICO ---
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "💳 Forma de Pagamento:");
        static int formaPagamento = 0; // 0: Pix, 1: Cartão, 2: Dinheiro

        static float valorPagoDinheiro = 0.0f;

        ImGui::RadioButton("📱 Pix", &formaPagamento, 0);
        ImGui::SameLine();
        ImGui::RadioButton("💳 Cartão", &formaPagamento, 1);
        ImGui::SameLine();
        ImGui::RadioButton("💵 Dinheiro", &formaPagamento, 2);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float valorTotalFinal = valorServico + totalProdutosConsumidos;

        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "VALOR TOTAL DA COMANDA: R$ %.2f", valorTotalFinal);
        ImGui::Spacing();

        // Painel Orientativo Dinâmico
        if (formaPagamento == 2) // DINHEIRO
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.14f, 1.0f));
            ImGui::BeginChild("PainelDinheiro", ImVec2(0, 80), true, ImGuiWindowFlags_NoScrollbar);

            ImGui::Text("Valor Recebido em Dinheiro:");
            ImGui::SetNextItemWidth(150.0f);

            if (valorPagoDinheiro < valorTotalFinal && valorPagoDinheiro == 0.0f)
            {
                valorPagoDinheiro = valorTotalFinal;
            }

            ImGui::InputFloat("##valorPagoInput", &valorPagoDinheiro, 5.0f, 10.0f, "%.2f");

            float troco = valorPagoDinheiro - valorTotalFinal;
            if (troco >= 0.0f)
            {
                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "💵 TROCO A DEVOLVER: R$ %.2f", troco);
            }
            else
            {
                ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "⚠️ Valor insuficiente! Faltam R$ %.2f", -troco);
            }

            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
        else if (formaPagamento == 1) // CARTÃO
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.15f, 0.2f, 1.0f));
            ImGui::BeginChild("PainelCartao", ImVec2(0, 65), true, ImGuiWindowFlags_NoScrollbar);

            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "💳 INSTRUÇÃO DE COBRANÇA:");
            ImGui::Text("Insira ou passe o cartão na maquininha no valor de:");
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "R$ %.2f", valorTotalFinal);

            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
        else if (formaPagamento == 0) // PIX
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.12f, 0.2f, 1.0f));
            ImGui::BeginChild("PainelPix", ImVec2(0, 65), true, ImGuiWindowFlags_NoScrollbar);

            ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), "📱 COBRANÇA VIA PIX:");
            ImGui::Text("Valor a solicitar ao cliente: R$ %.2f", valorTotalFinal);
            ImGui::TextDisabled("(O QR Code para leitura será exibido ao clicar em Finalizar)");

            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();

        // --- 5. AÇÕES DE FINALIZAÇÃO ---
        bool pagamentoValido = (formaPagamento != 2) || (valorPagoDinheiro >= valorTotalFinal);

        if (!pagamentoValido)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("✅ Confirmar e Finalizar", ImVec2(200, 36)))
        {
            sqlite3 *dbAtivo = (db != nullptr) ? db : ::db;

            // Baixa no estoque protegida e registro de lucro no histórico oficial
            for (const auto &item : comandaDaCadeira)
            {
                if (item.produto_id > 0)
                {
                    double precoCusto = 0.0;
                    double precoVenda = 0.0;

                    std::string sqlPreco = "SELECT custo, venda FROM estoque_barbearia WHERE id = " + std::to_string(item.produto_id) + ";";
                    sqlite3_stmt *stmtP = nullptr;
                    if (sqlite3_prepare_v2(dbAtivo, sqlPreco.c_str(), -1, &stmtP, nullptr) == SQLITE_OK)
                    {
                        if (sqlite3_step(stmtP) == SQLITE_ROW)
                        {
                            precoCusto = sqlite3_column_double(stmtP, 0);
                            precoVenda = sqlite3_column_double(stmtP, 1);
                        }
                        sqlite3_finalize(stmtP);
                    }

                    double lucroTransacao = (precoVenda - precoCusto) * item.quantidade;

                    // 2. BAIXA PROTEGIDA NO BANCO (MAX(0, estoque - X))
                    std::string sqlBaixaEstoque = "UPDATE estoque_barbearia SET estoque = MAX(0, estoque - " + std::to_string(item.quantidade) +
                                                  ") WHERE id = " + std::to_string(item.produto_id) + " AND (tipo != 'Servico');";
                    sqlite3_exec(dbAtivo, sqlBaixaEstoque.c_str(), nullptr, nullptr, nullptr);

                    // 3. Registra na tabela historico_vendas
                    std::string sqlHist = "INSERT INTO historico_vendas (item_id, quantidade, lucro_total) VALUES (" +
                                          std::to_string(item.produto_id) + ", " +
                                          std::to_string(item.quantidade) + ", " +
                                          std::to_string(lucroTransacao) + ");";
                    sqlite3_exec(dbAtivo, sqlHist.c_str(), nullptr, nullptr, nullptr);
                }
            }

            // REGISTRO DO SERVIÇO PRINCIPAL NO HISTÓRICO
            std::string nomeClienteAgendado = "Cliente";
            std::string observacaoServico = "Servico Principal";

            std::string sqlBuscaCliente = "SELECT cliente_nome, servico FROM agendamentos WHERE barbeiro_id = " +
                                          std::to_string(ctx.cadeiraSelecionadaID) + " ORDER BY data_hora ASC LIMIT 1;";
            sqlite3_stmt *stmtBusca = nullptr;
            if (sqlite3_prepare_v2(dbAtivo, sqlBuscaCliente.c_str(), -1, &stmtBusca, nullptr) == SQLITE_OK)
            {
                if (sqlite3_step(stmtBusca) == SQLITE_ROW)
                {
                    const unsigned char *valNome = sqlite3_column_text(stmtBusca, 0);
                    const unsigned char *valObs = sqlite3_column_text(stmtBusca, 1);
                    if (valNome)
                        nomeClienteAgendado = reinterpret_cast<const char *>(valNome);
                    if (valObs)
                        observacaoServico = reinterpret_cast<const char *>(valObs);
                }
                sqlite3_finalize(stmtBusca);
            }

            std::string sqlHistServico = "INSERT INTO faturamento_historico (barbeiro_id, cliente, servico, valor, data) VALUES (" +
                                         std::to_string(ctx.cadeiraSelecionadaID) + ", '" +
                                         nomeClienteAgendado + "', '" +
                                         observacaoServico + "', " +
                                         std::to_string(valorServico) + ", datetime('now', 'localtime'));";

            sqlite3_exec(dbAtivo, sqlHistServico.c_str(), nullptr, nullptr, nullptr);

            // Registra venda no caixa
            const char *strFormaPag = (formaPagamento == 0) ? "Pix" : (formaPagamento == 1) ? "Cartao"
                                                                                            : "Dinheiro";
            std::string sqlCaixa = "INSERT INTO vendas (barbeiro_id, valor_total, forma_pagamento, data_hora) VALUES (" +
                                   std::to_string(ctx.cadeiraSelecionadaID) + ", " +
                                   std::to_string(valorTotalFinal) + ", '" +
                                   std::string(strFormaPag) + "', datetime('now', 'localtime'));";
            sqlite3_exec(dbAtivo, sqlCaixa.c_str(), nullptr, nullptr, nullptr);

            // Conclui o agendamento
            std::string sqlConcluirAgendamento =
                "DELETE FROM agendamentos WHERE id = ("
                "   SELECT id FROM agendamentos "
                "   WHERE barbeiro_id = " +
                std::to_string(ctx.cadeiraSelecionadaID) + " "
                                                           "   ORDER BY data_hora ASC LIMIT 1"
                                                           ");";
            sqlite3_exec(dbAtivo, sqlConcluirAgendamento.c_str(), nullptr, nullptr, nullptr);

            // Reseta status da cadeira para Livre
            std::string sqlLiberar = "UPDATE barbeiros SET status_atendimento = 'Livre' WHERE id = " + std::to_string(ctx.cadeiraSelecionadaID) + ";";
            sqlite3_exec(dbAtivo, sqlLiberar.c_str(), nullptr, nullptr, nullptr);

            if (formaPagamento == 0)
            {
                ctx.modalReceberPix = true;
            }

            comandaDaCadeira.clear();
            ctx.modalFechamento = false;
            ultimoBarbeiroCarregado = -1;
            valorPagoDinheiro = 0.0f;
        }

        if (!pagamentoValido)
        {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancelar", ImVec2(100, 36)))
        {
            ctx.modalFechamento = false;
        }

        // --- MODAL DE ALERTA DE ESTOQUE DA COMANDA ---
        if (mostrarAvisoEstoqueComanda)
        {
            ImGui::OpenPopup("Aviso de Estoque##ModalComandaEstoque");
            mostrarAvisoEstoqueComanda = false;
        }

        if (ImGui::BeginPopupModal("Aviso de Estoque##ModalComandaEstoque", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "⚠️ Estoque Insuficiente!");
            ImGui::Spacing();
            ImGui::Text("%s", msgErroEstoqueComanda.c_str());
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            if (ImGui::Button("Entendido", ImVec2(130, 35)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::End();
    }
}

bool SelecionarImagemQrCode(char *outCaminho, size_t maxTam)
{
    OPENFILENAMEA ofn;
    char szFile[260] = {0};

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Imagens (*.png;*.jpg;*.jpeg)\0*.png;*.jpg;*.jpeg\0Todos os Arquivos (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn) == TRUE)
    {
        strncpy(outCaminho, ofn.lpstrFile, maxTam - 1);
        return true;
    }
    return false;
}

GLuint CarregarTexturaImagem(const char *filename, int *out_width, int *out_height)
{
    int image_width = 0;
    int image_height = 0;
    unsigned char *image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return 0;

    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    if (out_width)
        *out_width = image_width;
    if (out_height)
        *out_height = image_height;

    return image_texture;
}

void CopiarImagemParaClipboard(const char *caminhoArquivo)
{
    if (!caminhoArquivo || strlen(caminhoArquivo) == 0)
        return;

    int width = 0, height = 0, channels = 0;
    // Carrega os pixels brutos da imagem via stb_image (RGB - 3 canais)
    unsigned char *imgData = stbi_load(caminhoArquivo, &width, &height, &channels, 3);
    if (!imgData)
        return;

    // Converte os dados RGBA/RGB para o formato Device Independent Bitmap (DIB) do Windows
    int rowSize = ((width * 3 + 3) & ~3); // Alinhamento de 4 bytes por linha no Windows
    DWORD dataSize = rowSize * height;
    DWORD totalSize = sizeof(BITMAPINFOHEADER) + dataSize;

    HGLOBAL hMem = GlobalAlloc(GHND, totalSize);
    if (hMem)
    {
        BYTE *pMem = (BYTE *)GlobalLock(hMem);
        if (pMem)
        {
            BITMAPINFOHEADER *bmi = (BITMAPINFOHEADER *)pMem;
            bmi->biSize = sizeof(BITMAPINFOHEADER);
            bmi->biWidth = width;
            bmi->biHeight = -height; // Inverte para ordem top-down padrão
            bmi->biPlanes = 1;
            bmi->biBitCount = 24;
            bmi->biCompression = BI_RGB;
            bmi->biSizeImage = dataSize;

            BYTE *pPixels = pMem + sizeof(BITMAPINFOHEADER);
            for (int y = 0; y < height; y++)
            {
                BYTE *srcRow = imgData + (y * width * 3);
                BYTE *destRow = pPixels + (y * rowSize);
                for (int x = 0; x < width; x++)
                {
                    // Converte RGB (stb_image) para BGR (Windows Bitmap)
                    destRow[x * 3 + 0] = srcRow[x * 3 + 2]; // B
                    destRow[x * 3 + 1] = srcRow[x * 3 + 1]; // G
                    destRow[x * 3 + 2] = srcRow[x * 3 + 0]; // R
                }
            }
            GlobalUnlock(hMem);

            // Grava o Bitmap na Área de Transferência
            if (OpenClipboard(NULL))
            {
                EmptyClipboard();
                SetClipboardData(CF_DIB, hMem);
                CloseClipboard();
            }
            else
            {
                GlobalFree(hMem);
            }
        }
        else
        {
            GlobalFree(hMem);
        }
    }

    stbi_image_free(imgData);
}

// ==========================================
// 4. FUNÇÃO PRINCIPAL DE RENDERIZAÇÃO DE PRODUTOS SERVIÇOS E ESTOQUE
// ==========================================

void RenderizarModalExcluirEstoque(sqlite3 *db, int usuario_id, int &idParaExcluir)
{
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("ModalConfirmarExclusaoEstoque", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "⚠️ Confirmar Exclusao de Item");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Deseja realmente remover este produto do seu estoque?");
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "✔ O historico de lucros e vendas registradas sera PRESERVADO.");
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("Sim, Excluir do Estoque", ImVec2(180, 30)))
        {

            if (idParaExcluir != -1)
            {
                sqlite3 *dbAtivo = (db != nullptr) ? db : ::db;

                if (dbAtivo)
                {
                    // 1. Desativa no estoque
                    std::string sqlSoft = "UPDATE estoque_barbearia SET ativo = 0 WHERE id = " + std::to_string(idParaExcluir) + ";";
                    sqlite3_exec(dbAtivo, sqlSoft.c_str(), nullptr, nullptr, nullptr);

                    // 2. Apaga o registro
                    std::string sqlDel = "DELETE FROM estoque_barbearia WHERE id = " + std::to_string(idParaExcluir) + ";";
                    sqlite3_exec(dbAtivo, sqlDel.c_str(), nullptr, nullptr, nullptr);

                    // 3. Limpa das comandas abertas na memória
                    for (auto &pair : g_ComandasPorCadeira)
                    {
                        auto &itens = pair.second;
                        for (auto it = itens.begin(); it != itens.end();)
                        {
                            if (it->produto_id == idParaExcluir)
                                it = itens.erase(it);
                            else
                                ++it;
                        }
                    }
                }

                idParaExcluir = -1; // Zera a variável que foi passada por referência (&)
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        if (ImGui::Button("Cancelar", ImVec2(100, 30)))
        {
            idParaExcluir = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void CarregarHistoricoCadeira(sqlite3 *db, int cadeiraID, int dias)
{
    listaHistoricoCadeira.clear();
    faturamentoTotalHistorico = 0.0f;
    faturamentoDiarioCadeira = 0.0f;

    std::string queryHist;

    // Se dias > 0 aplica o filtro no SQLite, senão traz tudo
    if (dias > 0)
    {
        queryHist = "SELECT cliente, servico, valor, data FROM faturamento_historico "
                    "WHERE barbeiro_id = ? AND data >= date('now', '-' || ? || ' days') "
                    "ORDER BY id DESC;";
    }
    else
    {
        queryHist = "SELECT cliente, servico, valor, data FROM faturamento_historico "
                    "WHERE barbeiro_id = ? ORDER BY id DESC;";
    }

    sqlite3_stmt *stmtHist = nullptr;

    if (sqlite3_prepare_v2(db, queryHist.c_str(), -1, &stmtHist, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmtHist, 1, cadeiraID);

        if (dias > 0)
        {
            sqlite3_bind_int(stmtHist, 2, dias);
        }

        while (sqlite3_step(stmtHist) == SQLITE_ROW)
        {
            RegistroHistorico reg;
            const char *cli = (const char *)sqlite3_column_text(stmtHist, 0);
            const char *srv = (const char *)sqlite3_column_text(stmtHist, 1);
            double val = sqlite3_column_double(stmtHist, 2);
            const char *dt = (const char *)sqlite3_column_text(stmtHist, 3);

            reg.cliente = cli ? cli : "";
            reg.servico = srv ? srv : "";
            reg.valor = (float)val;
            reg.data = dt ? dt : "";

            faturamentoTotalHistorico += reg.valor;

            // Verifica se a data do atendimento corresponde ao dia de hoje (considerando os primeiros 10 caracteres da string de data "AAAA-MM-DD")
            if (reg.data.length() >= 10)
            {
                // Pega a data de hoje do sistema em formato string para comparar
                time_t agora = time(0);
                tm *ltm = localtime(&agora);
                char hojeStr[11];
                sprintf(hojeStr, "%04d-%02d-%02d", 1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday);

                if (reg.data.substr(0, 10) == std::string(hojeStr))
                {
                    faturamentoDiarioCadeira += reg.valor;
                }
            }

            listaHistoricoCadeira.push_back(reg);
        }
        sqlite3_finalize(stmtHist);
    }
}

void RenderizarModalAlertaAtraso()
{
    if (ctx.modalAlertaAtraso)
    {
        ImGui::OpenPopup("Atenção: Atraso Pendente##ModalAlerta");
    }

    if (ImGui::BeginPopupModal("Atenção: Atraso Pendente##ModalAlerta", &ctx.modalAlertaAtraso, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "CRÍTICO: Cadeira Travada!");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Esta cadeira não pode ser liberada manualmente porque existe um");
        ImGui::Text("agendamento anterior que ainda não foi concluído ou excluído.");
        ImGui::Spacing();
        ImGui::TextDisabled("Ação necessária:");
        ImGui::Text("1. Clique em 'Gerenciar / Agendar' na cadeira.");
        ImGui::Text("2. Clique em 'Feito' ou 'Excluir' no agendamento pendente.");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Entendido", ImVec2(120, 0)))
        {
            ctx.modalAlertaAtraso = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// ==========================================
// BOTÃO DE TEMA DO CRM //
// ==========================================

void AplicarTema(TemaCRM tema)
{
    temaFundoAtual = (int)tema; // Sincroniza o índice da foto de fundo!

    ImGuiStyle &style = ImGui::GetStyle();
    ImVec4 *colors = style.Colors;

    // --- GEOMETRIA GLOBAL ---
    style.WindowRounding = 6.0f;
    style.ChildRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 6.0f;
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 6);
    style.FrameBorderSize = 1.0f;

    switch (tema)
    {
    case TEMA_DARK_BARBER:
        colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.85f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.95f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_Header] = ImVec4(0.18f, 0.30f, 0.45f, 0.90f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.24f, 0.38f, 0.55f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.20f, 0.35f, 0.52f, 0.90f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.45f, 0.65f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.28f, 0.42f, 1.00f);
        colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.20f, 0.90f);
        colors[ImGuiCol_Border] = ImVec4(0.25f, 0.35f, 0.45f, 0.50f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.18f, 0.30f, 0.45f, 1.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);
        break;

    case TEMA_CLASSIC_WOOD:
        colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.09f, 0.07f, 0.85f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.09f, 0.07f, 0.95f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_Header] = ImVec4(0.35f, 0.22f, 0.15f, 0.90f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.45f, 0.28f, 0.18f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.42f, 0.26f, 0.16f, 0.90f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.55f, 0.34f, 0.20f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.32f, 0.20f, 0.12f, 1.00f);
        colors[ImGuiCol_Text] = ImVec4(0.92f, 0.88f, 0.82f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.50f, 0.45f, 1.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.15f, 0.12f, 0.90f);
        colors[ImGuiCol_Border] = ImVec4(0.45f, 0.30f, 0.20f, 0.50f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.35f, 0.22f, 0.15f, 1.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);
        break;

    case TEMA_NEON_CYBER:
        colors[ImGuiCol_WindowBg] = ImVec4(0.03f, 0.03f, 0.05f, 0.85f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.03f, 0.03f, 0.06f, 0.95f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_Header] = ImVec4(0.00f, 0.45f, 0.45f, 0.90f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.60f, 0.55f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.00f, 0.55f, 0.48f, 0.90f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.00f, 0.75f, 0.60f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.42f, 0.36f, 1.00f);
        colors[ImGuiCol_Text] = ImVec4(0.90f, 1.00f, 0.98f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.30f, 0.60f, 0.55f, 1.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.12f, 0.14f, 0.90f);
        colors[ImGuiCol_Border] = ImVec4(0.00f, 0.75f, 0.65f, 0.50f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.00f, 1.00f, 0.80f, 0.04f);
        break;

    case TEMA_LIGHT_MINIMAL:                                            // --- TEMA CADEIRA (COM MODAIS ESCUROS E LEITURA PERFEITA) ---
        colors[ImGuiCol_WindowBg] = ImVec4(0.14f, 0.16f, 0.20f, 0.92f); // Fundo escuro azulado elegante
        colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.14f, 0.18f, 0.96f);  // Modais escuros
        colors[ImGuiCol_ChildBg] = ImVec4(0.09f, 0.10f, 0.13f, 0.60f);  // Cards e subseções

        colors[ImGuiCol_TitleBg] = ImVec4(0.18f, 0.25f, 0.35f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.32f, 0.45f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.25f, 0.36f, 0.50f, 0.90f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.32f, 0.45f, 0.60f, 1.00f);

        // Botões padrão do tema (Azul aço nítido)
        colors[ImGuiCol_Button] = ImVec4(0.22f, 0.34f, 0.48f, 0.90f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.44f, 0.60f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.16f, 0.26f, 0.38f, 1.00f);

        // Texto BRANCO para dar leitura total às fontes verdes e amarelas
        colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.65f, 0.70f, 1.00f);

        // Inputs com fundo escuro sutil
        colors[ImGuiCol_FrameBg] = ImVec4(0.08f, 0.09f, 0.12f, 0.90f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.14f, 0.18f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.06f, 0.07f, 0.10f, 1.00f);

        // Bordas e Tabelas
        colors[ImGuiCol_Border] = ImVec4(0.30f, 0.40f, 0.50f, 0.50f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.20f, 0.30f, 0.42f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.30f, 0.40f, 0.50f, 0.80f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.20f, 0.25f, 0.32f, 0.50f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);
        break;
    }
}

// 1. Título Centralizado com Destaque
void RenderizarTituloModal(const char *texto)
{
    float larguraJanela = ImGui::GetContentRegionAvail().x;
    float larguraTexto = ImGui::CalcTextSize(texto).x;
    ImGui::SetCursorPosX((larguraJanela - larguraTexto) * 0.5f);
    ImGui::TextColored(ImVec4(0.3f, 0.85f, 1.0f, 1.0f), "%s", texto);
    ImGui::Spacing();
}

// 2. Botão de Sucesso/Confirmação (Verde elegante adaptável)
bool BotaoSucesso(const char *label, ImVec2 tamanho)
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.55f, 0.28f, 0.90f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.68f, 0.35f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.42f, 0.22f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 1.00f, 1.00f, 1.00f)); // <-- Texto Sempre Branco Nítido

    bool ret = ImGui::Button(label, tamanho);
    ImGui::PopStyleColor(4); // Alterado para 4
    return ret;
}

// 3. Botão Secundário / Cancelar
bool BotaoCancelar(const char *label, ImVec2 tamanho)
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.40f, 0.40f, 0.45f, 0.70f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.60f, 0.25f, 0.25f, 0.90f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.18f, 0.18f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 1.00f, 1.00f, 1.00f)); // <-- Texto Sempre Branco Nítido

    bool ret = ImGui::Button(label, tamanho);
    ImGui::PopStyleColor(4); // Alterado para 4
    return ret;
}

void CarregarFundosTemas()
{
    int w, h;
    texturasFundos[0] = CarregarTexturaImagem("assets/fundos/fundo_vintage.jpg", &w, &h);
    texturasFundos[1] = CarregarTexturaImagem("assets/fundos/fundo_ferramentas.jpg", &w, &h);
    texturasFundos[2] = CarregarTexturaImagem("assets/fundos/fundo_neon.jpg", &w, &h);
    texturasFundos[3] = CarregarTexturaImagem("assets/fundos/fundo_cadeira.jpg", &w, &h);
}