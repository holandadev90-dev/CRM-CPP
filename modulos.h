#ifndef MODULOS_H
#define MODULOS_H

#include <string>
#include <vector>

// Estrutura para itens extras (produtos consumidos ou serviços adicionais)
struct ItemConsumo
{
    std::string descricao;
    float preco = 0.0f;
};

// Estrutura principal do Atendimento (inclui os dados da Comanda e Fechamento)
struct Atendimento
{
    std::string nomeCliente;
    std::string servico; // Serviço principal (ex: "Corte")
    float valor = 0.0f;  // Valor do serviço principal
    std::string horario;

    std::string nomeBarbeiro;        // Barbeiro responsável
    std::vector<ItemConsumo> extras; // Cerveja, barba, pomada, etc.
    float desconto = 0.0f;
    int formaPagamento = 0; // 0: Pix, 1: Cartao, 2: Dinheiro
    bool finalizado = false;

    // Métodos auxiliares
    float CalcularTotal() const
    {
        float total = valor;
        for (const auto &item : extras)
        {
            total += item.preco;
        }
        float totalComDesconto = total - desconto;
        return (totalComDesconto > 0.0f) ? totalComDesconto : 0.0f;
    }

    void Resetar()
    {
        nomeCliente.clear();
        servico.clear();
        horario.clear();
        nomeBarbeiro.clear();
        extras.clear();
        valor = 0.0f;
        desconto = 0.0f;
        formaPagamento = 0;
        finalizado = false;
    }
};

// Representação de cada Cadeira no módulo da barbearia
struct Cadeira
{
    int id = 0;
    std::string nomeBarbeiro;
    bool ocupada = false;
    float faturamentoDia = 0.0f;
    int totalAtendimentosDia = 0;
    Atendimento atendimentoAtual;
};

// Declaracao das funcoes de interface (ImGui)
void RenderizarJanelaClientes(bool *p_open, int donoId, std::string tipoNegocio);
void RenderizarModuloCadeiras(std::vector<Cadeira> &cadeiras);

#endif // MODULOS_H