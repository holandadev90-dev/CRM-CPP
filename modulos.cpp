#include "modulos.h"
#include "crm_clientes.h"
#include "crm_produtos.h"
#include "imgui.h"
#include <cstring>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>

// =====================================================================
// MODULO: CLIENTES
// =====================================================================
void RenderizarJanelaClientes(bool *p_open, int donoId, std::string tipoNegocio)
{
    // Declara todas as variáveis estáticas necessárias no topo da função
    static char obsBuffer[256] = "";
    static char nome[128] = "";
    static char telefone[32] = "";
    static int clienteEditandoId = -1;
    static std::string ultimoNegocio = "";

    std::string tituloJanela = "Clientes e Registros - " + tipoNegocio;

    ImGui::SetNextWindowSize(ImVec2(720, 600), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(tituloJanela.c_str(), p_open, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }

    // Limpa os campos automaticamente caso o perfil/negócio tenha mudado
    if (ultimoNegocio != tipoNegocio)
    {
        memset(obsBuffer, 0, sizeof(obsBuffer));
        memset(nome, 0, sizeof(nome));
        memset(telefone, 0, sizeof(telefone));
        clienteEditandoId = -1;

        ultimoNegocio = tipoNegocio;
    }

    const char *diasDaSemana[] = {"Dom", "Seg", "Ter", "Qua", "Qui", "Sex", "Sab"};
    auto agora = std::chrono::system_clock::now();
    auto tempo_t = std::chrono::system_clock::to_time_t(agora);
    struct tm *tm_hoje = std::localtime(&tempo_t);

    char bufferHoje[64];
    std::strftime(bufferHoje, sizeof(bufferHoje), "%Y-%m-%d", tm_hoje);
    std::string dataHojeStr(bufferHoje);

    int anoH = tm_hoje->tm_year + 1900;
    int mesH = tm_hoje->tm_mon + 1;
    int diaH = tm_hoje->tm_mday;

    std::string diasValidos[7];
    for (int i = 0; i < 7; i++)
    {
        time_t proximoDia = tempo_t + (i * 24 * 60 * 60);
        struct tm *tm_aux = std::localtime(&proximoDia);
        char buffer[64];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", tm_aux);
        std::string strData(buffer);
        if (i == 0)
            strData += " (Hoje)";
        else
            strData += std::string(" (") + diasDaSemana[tm_aux->tm_wday] + ")";
        diasValidos[i] = strData;
    }

    static int diaSelecionadoIndex = 0;
    const char *horas[] = {"08", "09", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22"};
    const char *minutos[] = {"00", "30"};
    static int horaSel = 0;
    static int minSel = 0;
    static char msgRetorno[128] = "";
    static bool sucesso = false;

    // Cabeçalho dinâmico baseado no nicho
    if (clienteEditandoId == -1)
    {
        if (tipoNegocio == "Barbearia")
            ImGui::Text("Novo Agendamento / Cadastro de Cliente:");
        else if (tipoNegocio == "Lanchonete")
            ImGui::Text("Novo Pedido / Mesa ou Retirada:");
        else
            ImGui::Text("Novo Registro / Atendimento:");
    }
    else
    {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Editando Registro (ID: %d)", clienteEditandoId);
    }

    ImGui::InputText("Nome", nome, IM_ARRAYSIZE(nome));
    ImGui::InputText("Telefone", telefone, IM_ARRAYSIZE(telefone));

    // Rótulo de observações adaptativo usando o obsBuffer limpo
    if (tipoNegocio == "Barbearia")
    {
        ImGui::InputText("Observacoes (Corte/Barba)", obsBuffer, IM_ARRAYSIZE(obsBuffer));
    }
    else if (tipoNegocio == "Lanchonete")
    {
        ImGui::InputText("Observacoes (Pedido/Mesa)", obsBuffer, IM_ARRAYSIZE(obsBuffer));
    }
    else
    {
        ImGui::InputText("Observacoes", obsBuffer, IM_ARRAYSIZE(obsBuffer));
    }

    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::BeginCombo("##DataSemana", diasValidos[diaSelecionadoIndex].c_str()))
    {
        for (int i = 0; i < 7; i++)
        {
            bool is_selected = (diaSelecionadoIndex == i);
            if (ImGui::Selectable(diasValidos[i].c_str(), is_selected))
                diaSelecionadoIndex = i;
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    if (ImGui::BeginCombo("##Hora", horas[horaSel]))
    {
        for (int i = 0; i < IM_ARRAYSIZE(horas); i++)
        {
            if (ImGui::Selectable(horas[i], horaSel == i))
                horaSel = i;
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    ImGui::Text(":");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    if (ImGui::BeginCombo("##Minuto", minutos[minSel]))
    {
        for (int i = 0; i < IM_ARRAYSIZE(minutos); i++)
        {
            if (ImGui::Selectable(minutos[i], minSel == i))
                minSel = i;
        }
        ImGui::EndCombo();
    }

    ImGui::Spacing();

    const char *labelBotao = (clienteEditandoId == -1) ? "Agendar / Cadastrar" : "Salvar Reagendamento";

    if (ImGui::Button(labelBotao, ImVec2(180, 30)))
    {
        if (strlen(nome) == 0)
        {
            strcpy(msgRetorno, "Preencha ao menos o Nome.");
            sucesso = false;
        }
        else
        {
            std::string dataStr = diasValidos[diaSelecionadoIndex];
            std::string dataFormatada = dataStr.substr(0, 10);
            std::string horarioFormatado = std::string(horas[horaSel]) + ":" + minutos[minSel];
            std::string prefixoSlot = "[" + dataFormatada + " " + horarioFormatado + "]";

            auto clientesExistentes = listarClientesDoNegocio(donoId);
            bool horarioOcupado = false;
            for (auto &c_exist : clientesExistentes)
            {
                if (c_exist.getId() != clienteEditandoId && c_exist.getCpf().rfind(prefixoSlot, 0) == 0)
                {
                    horarioOcupado = true;
                    break;
                }
            }

            if (horarioOcupado)
            {
                strcpy(msgRetorno, "Erro: Este horario ja esta ocupado!");
                sucesso = false;
            }
            else
            {
                std::string erro;
                std::string agendamentoInfo = prefixoSlot + " " + std::string(obsBuffer);

                if (clienteEditandoId == -1)
                {
                    if (cadastrarClienteNegocio(donoId, agendamentoInfo, nome, obsBuffer, telefone, erro))
                    {
                        strcpy(msgRetorno, "Salvo com sucesso!");
                        sucesso = true;
                        memset(nome, 0, sizeof(nome));
                        memset(telefone, 0, sizeof(telefone));
                        memset(obsBuffer, 0, sizeof(obsBuffer));
                    }
                }
                else
                {
                    if (atualizarClienteNegocio(clienteEditandoId, agendamentoInfo, nome, obsBuffer, telefone, erro))
                    {
                        strcpy(msgRetorno, "Atualizado com sucesso!");
                        sucesso = true;
                        clienteEditandoId = -1;
                        memset(nome, 0, sizeof(nome));
                        memset(telefone, 0, sizeof(telefone));
                        memset(obsBuffer, 0, sizeof(obsBuffer));
                    }
                }
            }
        }
    }

    if (clienteEditandoId != -1)
    {
        ImGui::SameLine();
        if (ImGui::Button("Cancelar", ImVec2(100, 30)))
        {
            clienteEditandoId = -1;
            strcpy(msgRetorno, "Edicao cancelada.");
            sucesso = true;
            memset(nome, 0, sizeof(nome));
            memset(telefone, 0, sizeof(telefone));
            memset(obsBuffer, 0, sizeof(obsBuffer));
        }
    }

    if (strlen(msgRetorno) > 0)
    {
        if (sucesso)
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", msgRetorno);
        else
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", msgRetorno);
    }

    ImGui::Separator();
    ImGui::Spacing();

    static int filtroAgenda = 0;
    ImGui::Text("Filtrar Agenda:");
    ImGui::RadioButton("Apenas Hoje", &filtroAgenda, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Proximos 7 Dias", &filtroAgenda, 1);
    ImGui::SameLine();
    ImGui::RadioButton("Mostrar Todos", &filtroAgenda, 2);

    ImGui::Spacing();
    // Legendas de cores
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
    ImGui::Button("##v1", ImVec2(15, 15));
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::Text("Hoje");
    ImGui::SameLine(90.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 0.3f, 1.0f));
    ImGui::Button("##v2", ImVec2(15, 15));
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::Text("1-3 dias");
    ImGui::SameLine(180.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
    ImGui::Button("##v3", ImVec2(15, 15));
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::Text("4d+");
    ImGui::SameLine(250.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
    ImGui::Button("##v4", ImVec2(15, 15));
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::Text("Passados");
    ImGui::Spacing();

    auto clientes = listarClientesDoNegocio(donoId);

    if (ImGui::BeginTable("tabela_clientes", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 180)))
    {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 30.0f);
        ImGui::TableSetupColumn("Nome", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Telefone", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Data/Hora e Obs");
        ImGui::TableSetupColumn("Acoes", ImGuiTableColumnFlags_WidthFixed, 180.0f);
        ImGui::TableHeadersRow();

        for (auto &c : clientes)
        {
            std::string dataStr = c.getCpf() + " " + c.getEmail() + " " + c.getTelefone();
            std::string dataAgendamento = "";
            ImVec4 corTexto = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            bool temDataValida = false;
            int diffDias = -999;

            size_t posInicio = dataStr.find('[');
            if (posInicio != std::string::npos && dataStr.length() >= posInicio + 11)
            {
                dataAgendamento = dataStr.substr(posInicio + 1, 10);
                int anoA = 0, mesA = 0, diaA = 0;

                if (sscanf(dataAgendamento.c_str(), "%d-%d-%d", &anoA, &mesA, &diaA) == 3)
                {
                    auto toDays = [](int y, int m, int d)
                    {
                        if (m < 3)
                        {
                            y--;
                            m += 12;
                        }
                        return 365 * y + y / 4 - y / 100 + y / 400 + (153 * m - 457) / 5 + d;
                    };
                    diffDias = toDays(anoA, mesA, diaA) - toDays(anoH, mesH, diaH);
                    temDataValida = true;
                }
            }

            if (filtroAgenda == 0 && dataAgendamento != dataHojeStr)
                continue;
            if (filtroAgenda == 1 && (!temDataValida || diffDias < 0 || diffDias > 7))
                continue;

            if (temDataValida)
            {
                if (diffDias == 0)
                    corTexto = ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
                else if (diffDias >= 1 && diffDias <= 3)
                    corTexto = ImVec4(1.0f, 1.0f, 0.3f, 1.0f);
                else if (diffDias >= 4)
                    corTexto = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                else
                    corTexto = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", c.getId());
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(c.getNome().c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(c.getTelefone().c_str());
            ImGui::TableSetColumnIndex(3);
            ImGui::TextColored(corTexto, "%s", c.getCpf().c_str());

            ImGui::TableSetColumnIndex(4);
            ImGui::PushID(c.getId());
            if (ImGui::SmallButton("Feito"))
            {
                deletarClienteDoNegocio(donoId, c.getId());
                if (clienteEditandoId == c.getId())
                    clienteEditandoId = -1;
                strcpy(msgRetorno, "Concluido com sucesso!");
                sucesso = true;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Editar"))
            {
                clienteEditandoId = c.getId();
                strncpy(nome, c.getNome().c_str(), sizeof(nome) - 1);
                strncpy(telefone, c.getTelefone().c_str(), sizeof(telefone) - 1);
                strncpy(obsBuffer, c.getEmail().c_str(), sizeof(obsBuffer) - 1);
                strcpy(msgRetorno, "Modo de edicao ativado.");
                sucesso = true;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Excluir"))
            {
                deletarClienteDoNegocio(donoId, c.getId());
                if (clienteEditandoId == c.getId())
                    clienteEditandoId = -1;
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

void RenderizarModuloCadeiras(std::vector<Cadeira> &cadeiras)
{
    ImGui::Begin("Gerenciamento de Cadeiras - Barbearia");

    if (ImGui::BeginTable("TabelaCadeiras", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Cadeira / Barbeiro");
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("Cliente Atual");
        ImGui::TableSetupColumn("Faturamento (Dia)");
        ImGui::TableSetupColumn("Ações");
        ImGui::TableHeadersRow();

        for (auto &cad : cadeiras)
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Cadeira #%d (%s)", cad.id, cad.nomeBarbeiro.c_str());

            ImGui::TableSetColumnIndex(1);
            if (cad.ocupada)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Ocupada");
            }
            else
            {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Livre");
            }

            ImGui::TableSetColumnIndex(2);
            if (cad.ocupada)
            {
                ImGui::Text("%s", cad.atendimentoAtual.nomeCliente.c_str());
            }
            else
            {
                ImGui::Text("-");
            }

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("R$ %.2f", cad.faturamentoDia);

            ImGui::TableSetColumnIndex(4);
            std::string labelBotao = cad.ocupada ? "Finalizar##" + std::to_string(cad.id) : "Atender##" + std::to_string(cad.id);

            if (ImGui::Button(labelBotao.c_str()))
            {
                if (cad.ocupada)
                {
                    cad.faturamentoDia += cad.atendimentoAtual.valor;
                    cad.totalAtendimentosDia++;
                    cad.ocupada = false;
                    cad.atendimentoAtual = {};
                }
                else
                {
                    cad.ocupada = true;
                    cad.atendimentoAtual = {"Cliente Exemplo", "Corte + Barba", 45.0f, "Agora"};
                }
            }
        }
        ImGui::EndTable();
    }
    ImGui::End();
}