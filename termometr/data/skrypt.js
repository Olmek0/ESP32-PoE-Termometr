const xValues = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24];
const yValues = [17, 18, 19, 20.8, 21, 20.7, 20,21, 22, 25.3, 25.1, 27, 26, 26, 26.7, 27, 27.6, 28.5, 30, 28, 28.4, 26.8, 25.6, 25.2];

const xValues2 = Array.from({ length: 30 }, (_, i) => i + 1);
const yValues2 = [
  24.5, 28.3, 26.1, 20.7, 25.4, 29.8, 22.0, 21.3, 27.5, 24.9,
  26.8, 23.4, 20.1, 25.2, 22.6, 28.7, 24.3, 23.8, 29.1, 21.6,
  27.9, 26.5, 28.0, 23.9, 22.7, 29.3, 25.8, 20.9, 21.7, 27.1
];

let chartInstance = null;

function createCha(title, datax, datay) {
    const ctx = document.getElementById('TempCha').getContext('2d');
    
    // Destroy old chart if it exists
    if (chartInstance) {
        chartInstance.destroy();
    }

    // Create new chart
    chartInstance = new Chart(ctx, {
        type: 'line',
        data: {
            labels: datax,
            datasets: [{
                label: 'Temperatura (°C)',
                data: datay,
                borderColor: 'rgba(209, 78, 42, 1)',
                backgroundColor: 'rgba(209, 78, 42, 1)',
                tension: 0.5,
                fill: false,
                pointRadius: 0
            }]
        },
        options: {
            
            responsive: true,
            scales: {
                y: {
                    min: Math.floor(Math.min(...datay)) - 1,
                    max: Math.ceil(Math.max(...datay)) + 1,
                    ticks: {
                        color: 'black',
                        stepSize: 1
                    }
                },
                x: {
                    ticks: { color: 'black' },
                    grid: {
                        drawOnChartArea: false,
                        borderColor: 'black',
                        borderWidth: 1
                    }
                }
            },
            plugins: {
                title: {
                    display: true,
                    text: title,
                    font: { size: 35 },
                    padding: { top: 10 },
                    position: 'top'
                },
                legend: {
                    position: 'top',
                    align: 'start',
                    labels: {
                        color: 'black'
                    }
                }
            }
        }
    });
}
function toggleCha(title, datay, datax, clickedElement) {
    // Update active state
    document.querySelectorAll('.Navbar div').forEach(el => el.classList.remove('active'));
    clickedElement.classList.add('active');

    // Create chart
    createCha(title, datax, datay);
    
}

window.addEventListener('DOMContentLoaded', () => {
    const defaultTab = document.querySelector('.Navbar div.active');
    toggleCha('Dzisiaj', yValues, xValues, defaultTab);
});
