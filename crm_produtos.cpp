#include "crm_produtos.h"
#include "banco.h"
#include "imgui.h"
#include <cstring>
#include <cfloat>
#include "telas.h"

// Criação oficial das variáveis de exclusão para o linker achar
int itemParaExcluirID = 0;
bool dispararModalExcluirEstoque = false;
int g_idEstoqueExcluirModal = -1;

bool cadastrarProdutoNegocio(int donoId, const ProdutoServico &produto, std::string &mensagemErro)
{
    if (!db)
    {
        mensagemErro = "Banco de dados nao inicializado.";
        return false;
    }

    if (produto.tipo != "produto" && produto.tipo != "servico")
    {
        mensagemErro = "Tipo invalido: use 'produto' ou 'servico'.";
        return false;
    }

    sqlite3_stmt *stmt = nullptr;
    std::string sql = "INSERT INTO produtos_servicos "
                      "(nome, descricao, tipo, sku, preco_custo, preco_venda, "
                      "estoque_atual, estoque_minimo, ativo, usuario_id) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK)
    {
        mensagemErro = "Erro ao preparar cadastro.";
        return false;
    }

    sqlite3_bind_text(stmt, 1, produto.nome.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, produto.descricao.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, produto.tipo.c_str(), -1, SQLITE_TRANSIENT);

    if (produto.sku.empty())
        sqlite3_bind_null(stmt, 4);
    else
        sqlite3_bind_text(stmt, 4, produto.sku.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_double(stmt, 5, produto.precoCusto);
    sqlite3_bind_double(stmt, 6, produto.precoVenda);
    sqlite3_bind_int(stmt, 7, produto.estoqueAtual);
    sqlite3_bind_int(stmt, 8, produto.estoqueMinimo);
    sqlite3_bind_int(stmt, 9, produto.ativo ? 1 : 0);
    sqlite3_bind_int(stmt, 10, donoId);

    bool sucesso = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!sucesso)
    {
        mensagemErro = "SKU ja cadastrado ou dado invalido.";
    }
    sqlite3_finalize(stmt);
    return sucesso;
}

std::vector<ProdutoServico> listarProdutosDoNegocio(int donoId)
{
    std::vector<ProdutoServico> lista;
    if (!db)
        return lista;

    sqlite3_stmt *stmt = nullptr;
    std::string sql = "SELECT id, nome, descricao, tipo, sku, preco_custo, preco_venda, "
                      "estoque_atual, estoque_minimo, ativo FROM produtos_servicos "
                      "WHERE usuario_id = ? ORDER BY nome;";

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, donoId);
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            ProdutoServico p;
            p.id = sqlite3_column_int(stmt, 0);

            const unsigned char *nomeDb = sqlite3_column_text(stmt, 1);
            const unsigned char *descDb = sqlite3_column_text(stmt, 2);
            const unsigned char *tipoDb = sqlite3_column_text(stmt, 3);
            const unsigned char *skuDb = sqlite3_column_text(stmt, 4);

            p.nome = nomeDb ? reinterpret_cast<const char *>(nomeDb) : "";
            p.descricao = descDb ? reinterpret_cast<const char *>(descDb) : "";
            p.tipo = tipoDb ? reinterpret_cast<const char *>(tipoDb) : "produto";
            p.sku = skuDb ? reinterpret_cast<const char *>(skuDb) : "";

            p.precoCusto = sqlite3_column_double(stmt, 5);
            p.precoVenda = sqlite3_column_double(stmt, 6);
            p.estoqueAtual = sqlite3_column_int(stmt, 7);
            p.estoqueMinimo = sqlite3_column_int(stmt, 8);
            p.ativo = sqlite3_column_int(stmt, 9) != 0;

            lista.push_back(p);
        }
        sqlite3_finalize(stmt);
    }
    return lista;
}

bool atualizarEstoqueProduto(int donoId, int produtoId, int novoEstoque)
{
    if (!db)
        return false;

    sqlite3_stmt *stmt = nullptr;
    std::string sql = "UPDATE produtos_servicos SET estoque_atual = ? "
                      "WHERE id = ? AND usuario_id = ?;";

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(stmt, 1, novoEstoque);
    sqlite3_bind_int(stmt, 2, produtoId);
    sqlite3_bind_int(stmt, 3, donoId);

    bool sucesso = (sqlite3_step(stmt) == SQLITE_DONE) && (sqlite3_changes(db) > 0);
    sqlite3_finalize(stmt);
    return sucesso;
}

bool atualizarProdutoDoNegocio(int donoId, const ProdutoServico &produto)
{
    if (!db)
        return false;

    sqlite3_stmt *stmt = nullptr;
    std::string sql = "UPDATE produtos_servicos SET nome = ?, descricao = ?, "
                      "preco_custo = ?, preco_venda = ?, estoque_minimo = ?, ativo = ? "
                      "WHERE id = ? AND usuario_id = ?;";

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, produto.nome.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, produto.descricao.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, produto.precoCusto);
    sqlite3_bind_double(stmt, 4, produto.precoVenda);
    sqlite3_bind_int(stmt, 5, produto.estoqueMinimo);
    sqlite3_bind_int(stmt, 6, produto.ativo ? 1 : 0);
    sqlite3_bind_int(stmt, 7, produto.id);
    sqlite3_bind_int(stmt, 8, donoId);

    bool sucesso = (sqlite3_step(stmt) == SQLITE_DONE) && (sqlite3_changes(db) > 0);
    sqlite3_finalize(stmt);
    return sucesso;
}

bool deletarProdutoDoNegocio(int donoId, int produtoId)
{
    if (!db)
        return false;

    sqlite3_stmt *stmt = nullptr;
    std::string sql = "DELETE FROM produtos_servicos WHERE id = ? AND usuario_id = ?;";

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(stmt, 1, produtoId);
    sqlite3_bind_int(stmt, 2, donoId);

    bool sucesso = (sqlite3_step(stmt) == SQLITE_DONE) && (sqlite3_changes(db) > 0);
    sqlite3_finalize(stmt);
    return sucesso;
}

std::vector<ProdutoServico> listarTodosProdutosGeral(sqlite3 *dbConn)
{
    std::vector<ProdutoServico> lista;
    sqlite3 *dbAtivo = (dbConn != nullptr) ? dbConn : db;

    if (!dbAtivo)
        return lista;

    sqlite3_stmt *stmt = nullptr;
    std::string sql = "SELECT id, nome, descricao, tipo, sku, preco_custo, preco_venda, "
                      "estoque_atual, estoque_minimo, ativo FROM produtos_servicos ORDER BY nome;";

    if (sqlite3_prepare_v2(dbAtivo, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            ProdutoServico p;
            p.id = sqlite3_column_int(stmt, 0);

            const unsigned char *nomeDb = sqlite3_column_text(stmt, 1);
            const unsigned char *descDb = sqlite3_column_text(stmt, 2);
            const unsigned char *tipoDb = sqlite3_column_text(stmt, 3);
            const unsigned char *skuDb = sqlite3_column_text(stmt, 4);

            p.nome = nomeDb ? reinterpret_cast<const char *>(nomeDb) : "";
            p.descricao = descDb ? reinterpret_cast<const char *>(descDb) : "";
            p.tipo = tipoDb ? reinterpret_cast<const char *>(tipoDb) : "produto";
            p.sku = skuDb ? reinterpret_cast<const char *>(skuDb) : "";

            p.precoCusto = sqlite3_column_double(stmt, 5);
            p.precoVenda = sqlite3_column_double(stmt, 6);
            p.estoqueAtual = sqlite3_column_int(stmt, 7);
            p.estoqueMinimo = sqlite3_column_int(stmt, 8);
            p.ativo = sqlite3_column_int(stmt, 9) != 0;

            lista.push_back(p);
        }
        sqlite3_finalize(stmt);
    }
    return lista;
}

void RenderizarJanelaProdutos(bool *p_open, int usuario_id, std::string tipoNegocio)
{
    extern sqlite3 *db;

    // Se a flag estiver falsa, não desenha nada
    if (!p_open || !*p_open)
        return;

    // Controle do Popup Modal do ImGui
    std::string idModal = "Produtos e Serviços - " + tipoNegocio + "##ModalProdutosExclusivo";

    if (!ImGui::IsPopupOpen(idModal.c_str()))
    {
        ImGui::OpenPopup(idModal.c_str());
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(920, 710), ImGuiCond_Appearing);

    // Usa BeginPopupModal para BLOQUEAR totalmente cliques no fundo do CRM
    if (ImGui::BeginPopupModal(idModal.c_str(), p_open, ImGuiWindowFlags_NoCollapse))
    {
        static char nomeProd[64] = "";
        static float custoProd = 0.0f;
        static float vendaProd = 0.0f;
        static int qtdProd = 0;

        static int tipoSelecionado = 0;
        const char *tiposItens[] = {"Produto", "Consumivel", "Servico"};

        static bool abrirFormCadastro = false;
        static int idProdutoVendendo = 0;
        static int qtdVendaDesejada = 1;
        static int idProdutoRepondo = 0;
        static int qtdRepomDesejada = 1;
        static bool mostrarAvisoEstoqueInsuficiente = false;

        // Garantia de criação das tabelas no SQLite
        if (db != nullptr)
        {
            std::string sqlEstoque = "CREATE TABLE IF NOT EXISTS estoque_barbearia ("
                                     "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                     "usuario_id INTEGER, "
                                     "nome TEXT, "
                                     "tipo TEXT, "
                                     "custo REAL, "
                                     "venda REAL, "
                                     "estoque INTEGER, "
                                     "ativo INTEGER DEFAULT 1);";
            sqlite3_exec(db, sqlEstoque.c_str(), 0, 0, 0);
            sqlite3_exec(db, "ALTER TABLE estoque_barbearia ADD COLUMN ativo INTEGER DEFAULT 1;", 0, 0, 0);

            std::string sqlHistorico = "CREATE TABLE IF NOT EXISTS historico_vendas ("
                                       "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                       "item_id INTEGER, "
                                       "quantidade INTEGER, "
                                       "lucro_total REAL, "
                                       "data_venda DATETIME DEFAULT CURRENT_TIMESTAMP);";
            sqlite3_exec(db, sqlHistorico.c_str(), 0, 0, 0);
            sqlite3_exec(db, "UPDATE estoque_barbearia SET estoque = 0 WHERE estoque < 0;", 0, 0, 0);
        }

        // --- CÁLCULOS DE DESEMPENHO ---
        double lucroHistoricoTotalGeral = 0.0;
        double capitalEmEstoqueGeral = 0.0;
        if (db != nullptr)
        {
            std::string sqlLucro = "SELECT COALESCE(SUM(h.lucro_total), 0.0) "
                                   "FROM historico_vendas h "
                                   "INNER JOIN estoque_barbearia e ON h.item_id = e.id "
                                   "WHERE e.usuario_id = ?;";
            sqlite3_stmt *stmtLucro;
            if (sqlite3_prepare_v2(db, sqlLucro.c_str(), -1, &stmtLucro, nullptr) == SQLITE_OK)
            {
                sqlite3_bind_int(stmtLucro, 1, usuario_id);
                if (sqlite3_step(stmtLucro) == SQLITE_ROW)
                    lucroHistoricoTotalGeral = sqlite3_column_double(stmtLucro, 0);
                sqlite3_finalize(stmtLucro);
            }

            std::string sqlCapital = "SELECT COALESCE(SUM(custo * estoque), 0.0) FROM estoque_barbearia WHERE usuario_id = ? AND estoque > 0;";
            sqlite3_stmt *stmtCapital;
            if (sqlite3_prepare_v2(db, sqlCapital.c_str(), -1, &stmtCapital, nullptr) == SQLITE_OK)
            {
                sqlite3_bind_int(stmtCapital, 1, usuario_id);
                if (sqlite3_step(stmtCapital) == SQLITE_ROW)
                    capitalEmEstoqueGeral = sqlite3_column_double(stmtCapital, 0);
                sqlite3_finalize(stmtCapital);
            }
        }

        // --- BARRA SUPERIOR E TÍTULO CENTRALIZADO ---
        float largDisponivel = ImGui::GetContentRegionAvail().x;

        // Botão de fechar modal alinhado no canto esquerdo
        if (BotaoCancelar("← Voltar / Fechar Modal", ImVec2(180, 32)))
        {
            *p_open = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        const char *tituloDesempenho = "Visão Geral de Desempenho do Estoque";
        float largTituloDes = ImGui::CalcTextSize(tituloDesempenho).x;
        ImGui::SetCursorPosX((largDisponivel - largTituloDes) * 0.5f);
        ImGui::TextColored(ImVec4(0.3f, 0.85f, 1.0f, 1.0f), "%s", tituloDesempenho);

        ImGui::Spacing();

        // Métricas de Lucro e Capital investido centralizadas
        ImGui::SetCursorPosX((largDisponivel - 450.0f) * 0.5f);
        ImGui::Text("Lucro Histórico Total:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "R$ %.2f", lucroHistoricoTotalGeral);
        ImGui::SameLine(0, 30);
        ImGui::Text("Em Estoque:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "R$ %.2f", capitalEmEstoqueGeral);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Botão Novo Item
        if (BotaoSucesso(abrirFormCadastro ? "← Fechar Formulário" : "+ Cadastrar Novo Item", ImVec2(220, 35)))
        {
            abrirFormCadastro = !abrirFormCadastro;
            idProdutoVendendo = 0;
            idProdutoRepondo = 0;
        }

        // --- CARD DE FORMULÁRIO DE CADASTRO RÁPIDO ---
        if (abrirFormCadastro)
        {
            ImGui::Spacing();
            if (ImGui::BeginChild("PainelCadastroItem", ImVec2(0, 250), true, ImGuiChildFlags_AlwaysUseWindowPadding))
            {
                const char *subTituloForm = "Novo Item / Cadastro Rápido";
                float largSubForm = ImGui::CalcTextSize(subTituloForm).x;
                ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - largSubForm) * 0.5f);
                ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive], "%s", subTituloForm);
                ImGui::Spacing();

                if (ImGui::BeginTable("TabelaCadastroItem", 2, ImGuiTableFlags_SizingFixedFit, ImVec2(0, 0)))
                {
                    ImGui::TableSetupColumn("Rotulo", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                    ImGui::TableSetupColumn("Campo", ImGuiTableColumnFlags_WidthStretch);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Nome do Item:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::InputText("##inputNomeProd", nomeProd, IM_ARRAYSIZE(nomeProd));

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Tipo / Nicho:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::Combo("##comboTipoProd", &tipoSelecionado, tiposItens, IM_ARRAYSIZE(tiposItens));

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Preço de Custo (R$):");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::InputFloat("##inputCustoProd", &custoProd, 0.01f, 1.0f, "%.2f");

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Preço de Venda (R$):");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::InputFloat("##inputVendaProd", &vendaProd, 0.01f, 1.0f, "%.2f");

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Estoque Inicial:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::InputInt("##inputQtdProd", &qtdProd);

                    ImGui::EndTable();
                }

                ImGui::Spacing();
                float largCardForm = ImGui::GetContentRegionAvail().x;
                if (BotaoSucesso("Salvar Item no Banco##btnSalvarProd", ImVec2(largCardForm, 34)))
                {
                    if (strlen(nomeProd) > 0 && db != nullptr)
                    {
                        if (qtdProd < 0)
                            qtdProd = 0;

                        std::string sql = "INSERT INTO estoque_barbearia (usuario_id, nome, tipo, custo, venda, estoque, ativo) VALUES (?, ?, ?, ?, ?, ?, 1);";
                        sqlite3_stmt *stmt;
                        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
                        {
                            sqlite3_bind_int(stmt, 1, usuario_id);
                            sqlite3_bind_text(stmt, 2, nomeProd, -1, SQLITE_STATIC);
                            sqlite3_bind_text(stmt, 3, tiposItens[tipoSelecionado], -1, SQLITE_STATIC);
                            sqlite3_bind_double(stmt, 4, custoProd);
                            sqlite3_bind_double(stmt, 5, vendaProd);
                            sqlite3_bind_int(stmt, 6, qtdProd);
                            sqlite3_step(stmt);
                            sqlite3_finalize(stmt);
                        }

                        memset(nomeProd, 0, sizeof(nomeProd));
                        custoProd = 0.0f;
                        vendaProd = 0.0f;
                        qtdProd = 0;
                        abrirFormCadastro = false;
                    }
                }
            }
            ImGui::EndChild();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // --- ABAS DE NAVEGAÇÃO E TABELA DE PRODUTOS ---
        if (ImGui::BeginTabBar("AbasNichoEstoque"))
        {
            auto RenderizarTabelaPorTipo = [&](const char *filtroTipo)
            {
                if (ImGui::BeginTable(filtroTipo, 8, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 300)))
                {
                    ImGui::TableSetupScrollFreeze(0, 1);
                    ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 40);
                    ImGui::TableSetupColumn("Nome", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Custo", ImGuiTableColumnFlags_WidthFixed, 70);
                    ImGui::TableSetupColumn("Venda", ImGuiTableColumnFlags_WidthFixed, 70);
                    ImGui::TableSetupColumn("Estoque", ImGuiTableColumnFlags_WidthFixed, 95);
                    ImGui::TableSetupColumn("Qtd. Vendida", ImGuiTableColumnFlags_WidthFixed, 90);
                    ImGui::TableSetupColumn("Lucro Acumulado", ImGuiTableColumnFlags_WidthFixed, 115);
                    ImGui::TableSetupColumn("Ações", ImGuiTableColumnFlags_WidthFixed, 220.0f);
                    ImGui::TableHeadersRow();

                    if (db != nullptr)
                    {
                        std::string sqlSelect =
                            "SELECT e.id, e.nome, e.tipo, e.custo, e.venda, e.estoque, "
                            "COALESCE(SUM(h.quantidade), 0) as total_vendido, "
                            "COALESCE(SUM(h.lucro_total), 0.0) as lucro_total "
                            "FROM estoque_barbearia e "
                            "LEFT JOIN historico_vendas h ON e.id = h.item_id "
                            "WHERE e.usuario_id = ? AND e.ativo = 1 "
                            "GROUP BY e.id;";

                        sqlite3_stmt *stmtSelect;
                        if (sqlite3_prepare_v2(db, sqlSelect.c_str(), -1, &stmtSelect, nullptr) == SQLITE_OK)
                        {
                            sqlite3_bind_int(stmtSelect, 1, usuario_id);

                            while (sqlite3_step(stmtSelect) == SQLITE_ROW)
                            {
                                int idItem = sqlite3_column_int(stmtSelect, 0);
                                const unsigned char *nomeItem = sqlite3_column_text(stmtSelect, 1);
                                const unsigned char *tipoItem = sqlite3_column_text(stmtSelect, 2);
                                double custoItem = sqlite3_column_double(stmtSelect, 3);
                                double vendaItem = sqlite3_column_double(stmtSelect, 4);
                                int estoqueItem = sqlite3_column_int(stmtSelect, 5);
                                int totalVendido = sqlite3_column_int(stmtSelect, 6);
                                double lucroAcumulado = sqlite3_column_double(stmtSelect, 7);

                                std::string sTipo = tipoItem ? (const char *)tipoItem : "";
                                if (std::string(filtroTipo) != "Geral" && sTipo != filtroTipo)
                                    continue;

                                ImGui::TableNextRow();
                                ImGui::PushID(idItem);

                                ImGui::TableSetColumnIndex(0);
                                ImGui::Text("%d", idItem);

                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextUnformatted(nomeItem ? (const char *)nomeItem : "");

                                ImGui::TableSetColumnIndex(2);
                                ImGui::Text("R$ %.2f", custoItem);

                                ImGui::TableSetColumnIndex(3);
                                ImGui::Text("R$ %.2f", vendaItem);

                                ImGui::TableSetColumnIndex(4);
                                if (sTipo == "Servico")
                                    ImGui::TextDisabled("N/A");
                                else if (estoqueItem <= 0)
                                    ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.25f, 1.0f), "0 (ESGOTADO)");
                                else if (estoqueItem <= 3)
                                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%d (Baixo)", estoqueItem);
                                else
                                    ImGui::Text("%d", estoqueItem);

                                ImGui::TableSetColumnIndex(5);
                                ImGui::Text("%d un.", totalVendido);

                                ImGui::TableSetColumnIndex(6);
                                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "R$ %.2f", lucroAcumulado);

                                // Coluna 7: Ações com confirmação e cancelamento compactos
                                ImGui::TableSetColumnIndex(7);
                                std::string btnVender = "Vender##" + std::to_string(idItem);

                                if (estoqueItem <= 0 && sTipo != "Servico")
                                {
                                    ImGui::BeginDisabled();
                                    BotaoSucesso("Vender##btnDisabled", ImVec2(52, 24));
                                    ImGui::EndDisabled();
                                }
                                else
                                {
                                    if (BotaoSucesso(btnVender.c_str(), ImVec2(52, 24)))
                                    {
                                        idProdutoVendendo = idItem;
                                        qtdVendaDesejada = 1;
                                        idProdutoRepondo = 0;
                                        abrirFormCadastro = false;
                                    }
                                }

                                ImGui::SameLine();
                                std::string btnRepor = "Repor##" + std::to_string(idItem);
                                if (ImGui::Button(btnRepor.c_str(), ImVec2(50, 24)))
                                {
                                    idProdutoRepondo = idItem;
                                    qtdRepomDesejada = 1;
                                    idProdutoVendendo = 0;
                                    abrirFormCadastro = false;
                                }

                                ImGui::SameLine();
                                static int idAguardandoConfirmacao = -1;

                                if (idAguardandoConfirmacao == idItem)
                                {
                                    // Botão de Confirmação Compacto (Vermelho)
                                    if (BotaoCancelar("Sim?##btnCrt", ImVec2(48, 24)))
                                    {
                                        if (db != nullptr)
                                        {
                                            std::string sqlSoft = "UPDATE estoque_barbearia SET ativo = 0 WHERE id = " + std::to_string(idItem) + ";";
                                            sqlite3_exec(db, sqlSoft.c_str(), nullptr, nullptr, nullptr);
                                            std::string sqlDel = "DELETE FROM estoque_barbearia WHERE id = " + std::to_string(idItem) + ";";
                                            sqlite3_exec(db, sqlDel.c_str(), nullptr, nullptr, nullptr);
                                        }
                                        idAguardandoConfirmacao = -1;
                                    }

                                    ImGui::SameLine(0, 4);

                                    // Botão X de Cancelamento (Compacto e bem visível)
                                    if (ImGui::Button("X##btnCancDel", ImVec2(24, 24)))
                                    {
                                        idAguardandoConfirmacao = -1;
                                    }
                                }
                                else
                                {
                                    // Botão Excluir Padrão
                                    if (BotaoCancelar("Excluir##btnDel", ImVec2(54, 24)))
                                    {
                                        idAguardandoConfirmacao = idItem;
                                    }
                                }

                                ImGui::PopID();
                            }
                            sqlite3_finalize(stmtSelect);
                        }
                    }
                    ImGui::EndTable();
                }
            };

            if (ImGui::BeginTabItem("Geral"))
            {
                RenderizarTabelaPorTipo("Geral");
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Produtos"))
            {
                RenderizarTabelaPorTipo("Produto");
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Consumíveis"))
            {
                RenderizarTabelaPorTipo("Consumivel");
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Serviços"))
            {
                RenderizarTabelaPorTipo("Servico");
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        // --- SUB-PAINEL DE REGISTRO DE VENDA ---
        if (idProdutoVendendo > 0)
        {
            ImGui::Spacing();
            if (ImGui::BeginChild("CardVenda", ImVec2(0, 85), true, ImGuiChildFlags_AlwaysUseWindowPadding))
            {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Registrar Venda / Baixa (Item ID: %d)", idProdutoVendendo);
                ImGui::SetNextItemWidth(120.0f);
                ImGui::InputInt("Quantidade##qtdVenda", &qtdVendaDesejada);
                if (qtdVendaDesejada < 1)
                    qtdVendaDesejada = 1;

                ImGui::SameLine(0, 20);
                if (BotaoSucesso("Confirmar Venda##btnConfVenda", ImVec2(150, 32)))
                {
                    double custoItem = 0.0, vendaItem = 0.0;
                    int estoqueAtual = 0;

                    std::string sqlBuscaPrecos = "SELECT custo, venda, estoque FROM estoque_barbearia WHERE id = ? AND usuario_id = ?;";
                    sqlite3_stmt *stmtPreco;
                    if (sqlite3_prepare_v2(db, sqlBuscaPrecos.c_str(), -1, &stmtPreco, nullptr) == SQLITE_OK)
                    {
                        sqlite3_bind_int(stmtPreco, 1, idProdutoVendendo);
                        sqlite3_bind_int(stmtPreco, 2, usuario_id);
                        if (sqlite3_step(stmtPreco) == SQLITE_ROW)
                        {
                            custoItem = sqlite3_column_double(stmtPreco, 0);
                            vendaItem = sqlite3_column_double(stmtPreco, 1);
                            estoqueAtual = sqlite3_column_int(stmtPreco, 2);
                        }
                        sqlite3_finalize(stmtPreco);
                    }

                    if (qtdVendaDesejada > estoqueAtual || estoqueAtual <= 0)
                    {
                        mostrarAvisoEstoqueInsuficiente = true;
                    }
                    else
                    {
                        double lucroTransacao = (vendaItem - custoItem) * qtdVendaDesejada;

                        std::string sqlUpdate = "UPDATE estoque_barbearia SET estoque = MAX(0, estoque - ?) WHERE id = ? AND usuario_id = ? AND estoque >= ?;";
                        sqlite3_stmt *stmtUpdate;
                        if (sqlite3_prepare_v2(db, sqlUpdate.c_str(), -1, &stmtUpdate, nullptr) == SQLITE_OK)
                        {
                            sqlite3_bind_int(stmtUpdate, 1, qtdVendaDesejada);
                            sqlite3_bind_int(stmtUpdate, 2, idProdutoVendendo);
                            sqlite3_bind_int(stmtUpdate, 3, usuario_id);
                            sqlite3_bind_int(stmtUpdate, 4, qtdVendaDesejada);
                            sqlite3_step(stmtUpdate);
                            sqlite3_finalize(stmtUpdate);
                        }

                        std::string sqlHist = "INSERT INTO historico_vendas (item_id, quantidade, lucro_total) VALUES (?, ?, ?);";
                        sqlite3_stmt *stmtHist;
                        if (sqlite3_prepare_v2(db, sqlHist.c_str(), -1, &stmtHist, nullptr) == SQLITE_OK)
                        {
                            sqlite3_bind_int(stmtHist, 1, idProdutoVendendo);
                            sqlite3_bind_int(stmtHist, 2, qtdVendaDesejada);
                            sqlite3_bind_double(stmtHist, 3, lucroTransacao);
                            sqlite3_step(stmtHist);
                            sqlite3_finalize(stmtHist);
                        }
                    }
                    idProdutoVendendo = 0;
                }

                ImGui::SameLine();
                if (BotaoCancelar("Cancelar Venda##btnCancVenda", ImVec2(140, 32)))
                {
                    idProdutoVendendo = 0;
                }
            }
            ImGui::EndChild();
        }

        // --- SUB-PAINEL DE REPOSIÇÃO ---
        if (idProdutoRepondo > 0)
        {
            ImGui::Spacing();
            if (ImGui::BeginChild("CardRepor", ImVec2(0, 85), true, ImGuiChildFlags_AlwaysUseWindowPadding))
            {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.5f, 1.0f), "Repor Estoque (Item ID: %d)", idProdutoRepondo);
                ImGui::SetNextItemWidth(120.0f);
                ImGui::InputInt("Quantidade##qtdRepor", &qtdRepomDesejada);
                if (qtdRepomDesejada < 1)
                    qtdRepomDesejada = 1;

                ImGui::SameLine(0, 20);
                if (BotaoSucesso("Confirmar Reposição##btnConfRep", ImVec2(160, 32)))
                {
                    std::string sqlRepor = "UPDATE estoque_barbearia SET estoque = estoque + ? WHERE id = ? AND usuario_id = ?;";
                    sqlite3_stmt *stmtRepor;
                    if (sqlite3_prepare_v2(db, sqlRepor.c_str(), -1, &stmtRepor, nullptr) == SQLITE_OK)
                    {
                        sqlite3_bind_int(stmtRepor, 1, qtdRepomDesejada);
                        sqlite3_bind_int(stmtRepor, 2, idProdutoRepondo);
                        sqlite3_bind_int(stmtRepor, 3, usuario_id);
                        sqlite3_step(stmtRepor);
                        sqlite3_finalize(stmtRepor);
                    }
                    idProdutoRepondo = 0;
                }

                ImGui::SameLine();
                if (BotaoCancelar("Cancelar Reposição##btnCancRep", ImVec2(150, 32)))
                {
                    idProdutoRepondo = 0;
                }
            }
            ImGui::EndChild();
        }

        // --- MODAL DE AVISO DE ESTOQUE INSUFICIENTE ---
        if (mostrarAvisoEstoqueInsuficiente)
        {
            ImGui::OpenPopup("Estoque Insuficiente##AvisoModal");
            mostrarAvisoEstoqueInsuficiente = false;
        }

        if (ImGui::BeginPopupModal("Estoque Insuficiente##AvisoModal", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "⚠️ Operação Bloqueada!");
            ImGui::Spacing();
            ImGui::Text("Não foi possível realizar a venda pois o estoque está esgotado.\n\nDica: Clique em 'Repor' para adicionar unidades.");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (BotaoSucesso("Entendido (OK)", ImVec2(160, 35)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::EndPopup();
    }
    else
    {
        // Se a janela for fechada pelo 'X' nativo ou atalho, sincroniza a variável booleana
        *p_open = false;
    }
}