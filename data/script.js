const MAX_PONTOS = 20;

// 1. Função para criar gráficos mais rápido sem repetir código
function criarGrafico(idCanvas, corLinha, corFundo) {
    const ctx = document.getElementById(idCanvas).getContext('2d');
    return new Chart(ctx, {
        type: 'line',
        data: {
            labels: [], 
            datasets: [{
                data: [],
                borderColor: corLinha,
                backgroundColor: corFundo,
                borderWidth: 2,
                tension: 0.3, // Deixa a linha curvada
                fill: true    // Pinta a área abaixo da linha
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: { display: false } // Esconde a legenda, pois o título já está no HTML
            },
            scales: {
                y: {
                    // O 'false' permite que o gráfico dê "zoom" na variação 
                    // (útil para pressão atmosférica que varia muito pouco)
                    beginAtZero: false 
                }
            }
        }
    });
}

const chartTemp = criarGrafico('graficoTemp', '#e74c3c', 'rgba(231, 76, 60, 0.1)');
const chartUmid = criarGrafico('graficoUmid', '#3498db', 'rgba(52, 152, 219, 0.1)');
const chartPres = criarGrafico('graficoPres', '#f1c40f', 'rgba(241, 196, 15, 0.1)');
const chartAlti = criarGrafico('graficoAlti', '#9b59b6', 'rgba(155, 89, 182, 0.1)');

if (!!window.EventSource) {
    var source = new EventSource('/eventos');

    source.addEventListener('nova_leitura', function(e) {
        var dados = JSON.parse(e.data);
        
        var dataAtual = new Date();
        var hora = dataAtual.getHours() + ':' + 
                   dataAtual.getMinutes().toString().padStart(2, '0') + ':' + 
                   dataAtual.getSeconds().toString().padStart(2, '0');

        const graficos = [chartTemp, chartUmid, chartPres, chartAlti];
        const valores = [dados.temp, dados.humi, dados.pres, dados.alti];

        graficos.forEach((grafico, index) => {
            grafico.data.labels.push(hora);
            grafico.data.datasets[0].data.push(valores[index]);

            if (grafico.data.labels.length > MAX_PONTOS) {
                grafico.data.labels.shift();
                grafico.data.datasets[0].data.shift();
            }
            grafico.update();
        });
        
    }, false);

    source.addEventListener('error', function(e) {
        if (e.target.readyState != EventSource.OPEN) {
            console.log("Conexão perdida com o ESP32.");
        }
    }, false);

} else {
    alert("Seu navegador não suporta Server-Sent Events (SSE).");
}