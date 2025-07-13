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
    
    if (chartInstance) {
        chartInstance.destroy();
    }

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
    const chartCanvas = document.getElementById('TempCha');
    const timeElement = document.querySelector('.Time');
    const tempStatsElement = document.querySelector('.TempTimeStat');
    
    chartCanvas.classList.add('fade-out');
    timeElement.classList.add('fade-out');
    tempStatsElement.classList.add('fade-out');
    
    setTimeout(() => {
        document.querySelectorAll('.Navbar div').forEach(el => el.classList.remove('active'));
        clickedElement.classList.add('active');
        
        createCha(title, datax, datay);
        
        updateTemperatureStats();
        
        chartCanvas.classList.remove('fade-out');
        timeElement.classList.remove('fade-out');
        tempStatsElement.classList.remove('fade-out');
    }, 500);
}

window.addEventListener('DOMContentLoaded', () => {
    const defaultTab = document.querySelector('.Navbar div.active');
    toggleCha('Dzisiaj', yValues, xValues, defaultTab);
   
    const optionsBar = document.querySelector('.OptionsBar');
    const optionsMenu = document.getElementById('Options');
    
    optionsBar.addEventListener('click', function(e) {
        e.stopPropagation();
        optionsMenu.classList.toggle('show');
    });
    document.addEventListener('click', function() {
        optionsMenu.classList.remove('show');
        e.stopPropagation();
        optionsBar.classList.toggle('active');
    });
    document.addEventListener('keydown', function(e) {
        if (e.key === 'Escape') {
            optionsMenu.classList.remove('show');
        }
    });
});

let useFahrenheit = false;
let latestTempC = null;
let latestTempF = null;

let statData = {};

let socket;

try {
  const wsUrl = 'ws://' + location.hostname + ':81/';

  if (!location.hostname) {
    throw new Error("Invalid hostname for WebSocket.");
  }

  socket = new WebSocket(wsUrl);

  socket.onmessage = function(event) {
    try {
      const data = JSON.parse(event.data);
      WebSock(); 
      if (data.total !== undefined) {
        statData = data;
        updateStatsDisplay();
      } else {
        latestTempC = data.c;
        latestTempF = data.f;
        updateDisplayedTemp();
      }
    } catch (err) {
      console.error("Invalid JSON from socket:", err);
    }
  };

} catch (e) {
  console.warn("WebSocket not initialized:", e.message);
}


function getUnit() {
    const unit = useFahrenheit ? "°F" : "°C";
    return {
        unit,
        temp: useFahrenheit ? latestTempF : latestTempC,
        max: useFahrenheit ? statData.maxf : statData.max,
        min: useFahrenheit ? statData.minf : statData.min
    };
}

function updateStatsDisplay() {
    const { unit, max, min } = getUnit();
	
    document.getElementById("stat-total").textContent = `Ilość odczytów - ${statData.total}`;
    document.getElementById("stat-max").textContent = `Najwyższa temperatura - ${max}${unit}`;
    document.getElementById("stat-min").textContent = `Najniższa temperatura - ${min}${unit}`;
    document.getElementById("stat-start").textContent = `Odczyty wprowadzone od - ${statData.start}`;
}

function updateDisplayedTemp() {
	const { temp, unit } = getUnit();
    document.getElementById("temp").textContent = temp + unit;
}

function updateTemperatureStats() {
    const activeTab = document.querySelector('.Navbar div.active');
    let datay, title;
    
    if (activeTab.textContent.includes('Dzisiaj')) {
        datay = yValues;
        title = 'Dzisiaj';
    } else if (activeTab.textContent.includes('W miesiącu')) {
        datay = yValues2;
        title = 'W miesiącu';
    }
    
    const minTemp = Math.min(...datay);
    const maxTemp = Math.max(...datay);
    const avgTemp = (datay.reduce((a, b) => a + b, 0) / datay.length).toFixed(2);
    const unit = useFahrenheit ? '°F' : '°C';
    
    document.querySelector('.Time').textContent = title.toUpperCase();
    document.querySelector('.TempTimeStat').innerHTML = 
        `TEMPERATURA<br><span class="red">MIN: </span>${minTemp}${unit} | ` +
        `<span class="red">ŚRD: </span>${avgTemp}${unit} | ` +
        `<span class="red">MAKS:</span> ${maxTemp}${unit}`;
}

function WebSock() {
    WebSockA();
    document.getElementById("toggleUnit").addEventListener("click", () => {
        updateDisplayedTemp();
        updateStatsDisplay(); 
    });
}

function WebSockA() {
    document.getElementById("toggleUnit").addEventListener("click", () => {
        useFahrenheit = !useFahrenheit;
        const text = useFahrenheit ? "°F" : "°C";
        fadeText(document.getElementById("toggleUnit"), text);
        updateTemperatureStats()
    });
}

WebSockA();

function fadeText(element, newText) {
    const span = element.querySelector('span');

    span.classList.add('fade-out');
    
    setTimeout(() => {
        span.textContent = newText;
        span.classList.remove('fade-out');
        span.classList.add('fade-in');
        
        setTimeout(() => {
            span.classList.remove('fade-in');
        }, 300);
    }, 300);
}

let date = new Intl.DateTimeFormat("pl-PL", { 
	day: "2-digit", month: "2-digit", year: "numeric"
}).format(new Date());

document.getElementById("data").innerHTML = date;
