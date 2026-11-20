#include <iostream>
#include <string>
#include <cstring>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include "banco.h"
#include "telas.h"
#include "modulos.h"
#include "gerenciadorFinanceiro.h"

using namespace std;

std::vector<Cadeira> minhasCadeiras = {
    {1, "Barbeiro Carlos", false, 0.0f, 0, {}},
    {2, "Barbeiro João", false, 0.0f, 0, {}},
    {3, "Barbeiro Pedro", false, 0.0f, 0, {}}};

// Variáveis globais de controle
bool usuarioLogado = false;
bool showProdutosWindow = false;
GerenciadorFinanceiro gerenciadorFinanceiro;
bool abrir_modal_usuario = false;
static bool showAdvancedWindow = false;
static bool showManagerWindow = false;
static bool showClientesWindow = false;
static bool showFinanceiroWindow = false;
static bool exibirModalFechamento = false;
static char nomeExibicao[64] = "";
static int cadeiraSelecionadaIndex = -1;

std::string cargoAtual = "";
std::string nomeExibicaoAtual = "";
std::string tipoNegocioAtual = "";
int usuarioLogadoId = -1;

char loginUsuario[64] = "";
char loginSenha[64] = "";
char mensagemErro[128] = "";

int main()
{
    inicializarBanco();

    // CARREGAR DADOS DO PIX LOGO APÓS ABRIR O BANCO
    InicializarDadosPix(db);

    // Inicializa o GLFW
    if (!glfwInit())
        return -1;

    GLFWwindow *window = glfwCreateWindow(1280, 720, "CRM - Sistema de Clientes", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Ativa o V-Sync

    // Inicialização do ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();

    // 1. Inicializa os backends PRIMEIRO
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // 2. Agora o contexto OpenGL está pronto para carregar as fotos dos temas
    CarregarFundosTemas();
    AplicarTema(TEMA_DARK_BARBER);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 2. Renderização das janelas principais do ImGui
        if (!usuarioLogado)
        {
            ImGui::SetNextWindowSize(ImVec2(350, 220), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(200, 150), ImGuiCond_FirstUseEver);

            if (ImGui::Begin("Login - CRM", NULL, ImGuiWindowFlags_NoCollapse))
            {
                ImGui::Text("Faca seu login para continuar:");
                ImGui::Spacing();

                ImGui::InputText("Usuario", loginUsuario, IM_ARRAYSIZE(loginUsuario));
                ImGui::InputText("Senha", loginSenha, IM_ARRAYSIZE(loginSenha), ImGuiInputTextFlags_Password);
                ImGui::Spacing();

                if (ImGui::Button("Entrar", ImVec2(120, 30)))
                {
                    std::string cargoTemp = "";
                    std::string nomeExibicaoTemp = "";

                    if (autenticarUsuarioCompleto(loginUsuario, loginSenha, cargoTemp, nomeExibicaoTemp))
                    {
                        usuarioLogado = true;
                        cargoAtual = cargoTemp;
                        nomeExibicaoAtual = nomeExibicaoTemp;
                        usuarioLogadoId = obterIdUsuarioPorUsername(loginUsuario);
                        tipoNegocioAtual = obterTipoNegocioPorUsername(loginUsuario);
                        strcpy(mensagemErro, "");
                    }
                    else
                    {
                        strcpy(mensagemErro, "Usuario ou senha incorretos!");
                    }
                }

                if (strlen(mensagemErro) > 0)
                {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", mensagemErro);
                }
            }
            ImGui::End();
        }
        else
        {
            // Cria um título dinâmico com o nicho da empresa logada
            std::string tituloPainel = "Painel Principal - CRM (" + tipoNegocioAtual + ")";
            ImGui::Begin(tituloPainel.c_str(), NULL, ImGuiWindowFlags_NoCollapse);
            ImGui::SetWindowSize(ImVec2(650, 420), ImGuiCond_Once);

            // --- DESENHA A FOTO DE FUNDO DO TEMA ATUAL ---
            ImVec2 posWin = ImGui::GetWindowPos();
            ImVec2 sizeWin = ImGui::GetWindowSize();

            if (texturasFundos[temaFundoAtual] != 0)
            {
                ImGui::GetWindowDrawList()->AddImage(
                    (void *)(intptr_t)texturasFundos[temaFundoAtual],
                    posWin,
                    ImVec2(posWin.x + sizeWin.x, posWin.y + sizeWin.y),
                    ImVec2(0, 0), ImVec2(1, 1),
                    IM_COL32(255, 255, 255, 85) // Reduzido para 85 para dar mais destaque às fotos internas e textos
                );
            }

            // Mostra o nome, perfil e o nicho de forma clara
            ImGui::Text("Bem-vindo, %s! (Perfil: %s | Negocio: %s)", nomeExibicaoAtual.c_str(), cargoAtual.c_str(), tipoNegocioAtual.c_str());

            // --- BOTÃO DE ESTOQUE NO CANTO SUPERIOR DIREITO ---
            float larguraJanela = ImGui::GetWindowWidth();
            float larguraBotao = 210.0f;
            float margemDireita = 20.0f;

            ImGui::SameLine();
            ImGui::SetCursorPosX(larguraJanela - larguraBotao - margemDireita);

            if (ImGui::Button("? Gerenciar Estoque", ImVec2(210.0f, 30.0f)))
            {
                bool estavaAberto = showProdutosWindow;
                FecharTodasAsJanelasExclusivas();
                showProdutosWindow = !estavaAberto;
            }

            ImGui::Separator();
            ImGui::Spacing();

            if (cargoAtual != "ADMIN")
            {
                if (tipoNegocioAtual == "Barbearia")
                {
                    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "[Modulo Barbearia Ativo]");
                }
                else if (tipoNegocioAtual == "Restaurante" || tipoNegocioAtual == "Lanchonete")
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "[Modulo Alimentacao Ativo]");
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (tipoNegocioAtual == "Barbearia")
                {
                    CadeirasBarber();
                }
            }

            // SE FOR ADMIN: Mostra os botões de gerenciamento do sistema
            if (cargoAtual == "ADMIN")
            {
                if (ImGui::Button("Cadastrar Novo Usuario", ImVec2(200, 30)))
                {
                    abrir_modal_usuario = true;
                }

                ImGui::SameLine();

                if (ImGui::Button("Gerenciar Clientes / Sistema", ImVec2(220, 30)))
                {
                    showManagerWindow = true;
                }

                ImGui::SameLine();

                if (ImGui::Button("Configuracoes e Relatorios", ImVec2(220, 30)))
                {
                    showAdvancedWindow = true;
                }

                ImGui::Spacing();

                if (ImGui::Button("Gerenciador Financeiro", ImVec2(200, 30)))
                {
                    showFinanceiroWindow = true;
                }

                ImGui::Spacing();
            }

            // Botão de Logout comum a todos
            if (ImGui::Button("Sair / Logout", ImVec2(120, 30)))
            {
                usuarioLogado = false;
                cargoAtual = "";
                nomeExibicaoAtual = "";
                usuarioLogadoId = -1;
                tipoNegocioAtual = "";

                showClientesWindow = false;
                showProdutosWindow = false;
                showManagerWindow = false;
                showAdvancedWindow = false;
                showFinanceiroWindow = false;
                abrir_modal_usuario = false;

                memset(loginUsuario, 0, sizeof(loginUsuario));
                memset(loginSenha, 0, sizeof(loginSenha));
            }

            // Alinha no mesmo nível, no canto inferior
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 15.0f);

            // Botão menor/compacto para abrir a seleção de temas
            if (ImGui::Button("Tema", ImVec2(80, 30)))
            {
                ImGui::OpenPopup("PopupSeletorTema");
            }

            // Menu Popup discreto de Temas
            if (ImGui::BeginPopup("PopupSeletorTema"))
            {
                ImGui::TextDisabled("Selecione o Tema:");
                ImGui::Separator();

                static int temaSelecionado = 0;
                const char *nomesTemas[] = {"Barber Vintage", "Ferramentas", "Neon Cyber", "Cadeira Couro"};

                for (int i = 0; i < 4; i++)
                {
                    if (ImGui::Selectable(nomesTemas[i], temaSelecionado == i))
                    {
                        temaSelecionado = i;
                        AplicarTema((TemaCRM)temaSelecionado);
                    }
                }

                ImGui::EndPopup();
            }

            ImGui::End(); // Fecha a janela do Painel Principal
        }

        // --- JANELA PARA CADASTRAR NOVO USUÁRIO DO SISTEMA ---
        if (abrir_modal_usuario)
        {
            if (!usuarioLogado || cargoAtual != "ADMIN")
            {
                abrir_modal_usuario = false;
            }
            else
            {
                ImGui::SetNextWindowSize(ImVec2(400, 440), ImGuiCond_Once);
                if (ImGui::Begin("Cadastrar Acesso ao Sistema", &abrir_modal_usuario, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
                {
                    static char novoUser[64] = "";
                    static char novaSenha[64] = "";
                    static char nomeCompleto[128] = "";
                    static char cpfAdmin[32] = "";
                    static int tipoSelecionado = 0;
                    static int nichoSelecionado = 0;
                    static char msgRetorno[128] = "";
                    static bool sucesso = false;

                    ImGui::Text("Preencha os dados do novo acesso:");
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::InputText("Usuario / Login", novoUser, IM_ARRAYSIZE(novoUser));
                    ImGui::InputText("Senha", novaSenha, IM_ARRAYSIZE(novaSenha), ImGuiInputTextFlags_Password);
                    ImGui::InputText("Nome de Exibicao", nomeExibicaoAtual.data(), nomeExibicaoAtual.capacity());

                    ImGui::Spacing();
                    ImGui::Text("Tipo de Perfil:");
                    ImGui::RadioButton("Cliente", &tipoSelecionado, 0);
                    ImGui::SameLine();
                    ImGui::RadioButton("Administrador", &tipoSelecionado, 1);

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    if (tipoSelecionado == 0)
                    {
                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.5f, 1.0f), "Configuracao do Cliente:");
                        ImGui::Spacing();
                        ImGui::Text("Tipo de Negocio / Modulo:");
                        const char *nichos[] = {"Barbearia", "Restaurante", "Lanchonete", "Geral"};
                        ImGui::Combo("##nicho", &nichoSelecionado, nichos, IM_ARRAYSIZE(nichos));
                        ImGui::Spacing();
                    }
                    else
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Dados de Seguranca do Admin:");
                        ImGui::InputText("Nome Completo", nomeCompleto, IM_ARRAYSIZE(nomeCompleto));
                        ImGui::InputText("CPF / Identificacao", cpfAdmin, IM_ARRAYSIZE(cpfAdmin));
                        ImGui::Spacing();
                    }

                    ImGui::Separator();
                    ImGui::Spacing();

                    if (ImGui::Button("Salvar no Banco", ImVec2(140, 30)))
                    {
                        if (strlen(novoUser) > 0 && strlen(novaSenha) > 0)
                        {
                            std::string tipoStr = (tipoSelecionado == 1) ? "ADMIN" : "CLIENTE";
                            const char *nichosArr[] = {"Barbearia", "Restaurante", "Lanchonete", "Geral"};
                            std::string nichoStr = (tipoSelecionado == 0) ? nichosArr[nichoSelecionado] : "Sistema";
                            std::string cpfStr = (tipoSelecionado == 1) ? cpfAdmin : "";
                            std::string exibicaoStr = novoUser;

                            if (tipoSelecionado == 1 && cpfStr.empty())
                            {
                                strcpy(msgRetorno, "Erro: Informe o CPF do Administrador.");
                                sucesso = false;
                            }
                            else
                            {
                                if (cadastrarNovoUsuario(novoUser, novaSenha, tipoStr, exibicaoStr, nichoStr, cpfStr))
                                {
                                    strcpy(msgRetorno, "Sucesso! Usuario cadastrado.");
                                    sucesso = true;
                                    memset(novoUser, 0, sizeof(novoUser));
                                    memset(novaSenha, 0, sizeof(novaSenha));
                                    memset(nomeCompleto, 0, sizeof(nomeCompleto));
                                    memset(cpfAdmin, 0, sizeof(cpfAdmin));
                                }
                                else
                                {
                                    strcpy(msgRetorno, "Erro: Usuario ou CPF ja cadastrados!");
                                    sucesso = false;
                                }
                            }
                        }
                        else
                        {
                            strcpy(msgRetorno, "Preencha os campos obrigatorios!");
                            sucesso = false;
                        }
                    }

                    ImGui::SameLine();

                    if (ImGui::Button("Fechar", ImVec2(100, 30)))
                    {
                        abrir_modal_usuario = false;
                        memset(msgRetorno, 0, sizeof(msgRetorno));
                    }

                    if (strlen(msgRetorno) > 0)
                    {
                        ImGui::Spacing();
                        if (sucesso)
                            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", msgRetorno);
                        else
                            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", msgRetorno);
                    }
                }
                ImGui::End();
            }
        }

        // --- JANELA DE GERENCIAMENTO ---
        if (showManagerWindow)
        {
            RenderizarJanelaGerenciamento(&showManagerWindow);
        }

        // --- JANELAS DOS MÓDULOS DE CRM ---
        if (showClientesWindow)
        {
            RenderizarJanelaClientes(&showClientesWindow, usuarioLogadoId, tipoNegocioAtual);
        }

        if (showProdutosWindow)
        {
            RenderizarJanelaProdutos(&showProdutosWindow, usuarioLogadoId, tipoNegocioAtual);
        }

        // --- JANELA DE CONFIGURAÇÕES AVANÇADAS ---
        if (showAdvancedWindow)
        {
            RenderizarJanelaAvancada(&showAdvancedWindow);
        }

        // --- JANELA DO GERENCIADOR FINANCEIRO ---
        if (showFinanceiroWindow)
        {
            ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Gerenciador Financeiro", &showFinanceiroWindow, ImGuiWindowFlags_NoCollapse))
            {
                ImGui::Text("Resumo Financeiro do Sistema");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Total Bruto: R$ %.2f", gerenciadorFinanceiro.CalcularTotalBruto());
                ImGui::Text("Total Despesas: R$ %.2f", gerenciadorFinanceiro.CalcularTotalDespesas());
                ImGui::Text("Lucro Liquido: R$ %.2f", gerenciadorFinanceiro.CalcularLucroLiquido());

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::CollapsingHeader("Historico de Faturamento"))
                {
                    for (const auto &item : gerenciadorFinanceiro.ObterHistorico())
                    {
                        ImGui::BulletText("Cliente: %s | Servico: %s | R$ %.2f", item.cliente.c_str(), item.servico.c_str(), item.valor);
                    }
                }

                if (ImGui::CollapsingHeader("Lista de Despesas"))
                {
                    for (const auto &despesa : gerenciadorFinanceiro.ObterDespesas())
                    {
                        ImGui::BulletText("Tipo: %s | R$ %.2f", despesa.tipo.c_str(), despesa.valor);
                    }
                }

                ImGui::Spacing();
                if (ImGui::Button("Fechar Janela", ImVec2(120, 30)))
                {
                    showFinanceiroWindow = false;
                }
            }
            ImGui::End();
        }

        // --- MODAL GLOBAL DO PIX ---
        RenderizarModalReceberPix();
        RenderizarModalFechamento(db);

        // Renderização OpenGL / GLFW
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Encerramento
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}