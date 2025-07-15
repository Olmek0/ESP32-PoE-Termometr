
const xValues = Array.from({ length: 24 }, (_, i) => i + 1);;
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
                    labels: { color: 'black' }
                }
            }
        }
    });
}

let firstLoad = true;

function toggleCha(title, datay, datax, clickedElement) {
  document.querySelectorAll('.Navbar div').forEach(el => el.classList.remove('active'));
  clickedElement.classList.add('active');

  const chartCanvas = document.getElementById('TempCha');
  const timeElement = document.querySelector('.Time');
  const tempStatsElement = document.querySelector('.TempTimeStat');
  const searchForm = document.getElementById('historySearchForm');

  if (title === 'Historia') {
    searchForm.style.display = 'block';
    chartCanvas.style.display = 'none';
    timeElement.style.display = 'none';
    tempStatsElement.style.display = 'none';
  } else {
    searchForm.style.display = 'none';
    chartCanvas.style.display = 'block';
    timeElement.style.display = 'block';
    tempStatsElement.style.display = 'block';

    if (firstLoad) {
      createCha(title, datax, datay);
      updateTemperatureStats();
      firstLoad = false;
      return;
    }

    chartCanvas.classList.add('fade-out');
    timeElement.classList.add('fade-out');
    tempStatsElement.classList.add('fade-out');

    setTimeout(() => {
      createCha(title, datax, datay);
      updateTemperatureStats();

      chartCanvas.classList.remove('fade-out');
      timeElement.classList.remove('fade-out');
      tempStatsElement.classList.remove('fade-out');
    }, 500);
  }

  if (title === 'Historia') {
    document.getElementById('startDate').value = '';
    document.getElementById('endDate').value = '';
  }
}

let Bar, Menu;

function handleGlobalInteraction(e) {
    const isClickOutside = e.type === 'click' && !e.target.closest('.OptionsBar');
    const isEscapeKey = e.type === 'keydown' && e.key === 'Escape';

    if (isClickOutside || isEscapeKey) {
		Menu.classList.remove('show');
		Bar.classList.remove('active');
    }
}

window.addEventListener('DOMContentLoaded', () => {
    const defaultTab = document.querySelector('.Navbar div.active');
    toggleCha('Dzisiaj', yValues, xValues, defaultTab);

    Bar = document.querySelector('.OptionsBar');
    Menu = document.getElementById('Options');

	Bar.addEventListener('click', function(e) {
        e.stopPropagation();
        Menu.classList.toggle('show');
		Bar.classList.toggle('active');
    });

	document.addEventListener('click', handleGlobalInteraction);
	document.addEventListener('keydown', handleGlobalInteraction);

    document.getElementById("toggleUnit").addEventListener("click", () => {
        useFahrenheit = !useFahrenheit;
		localStorage.setItem("tempUnit", useFahrenheit ? "F" : "C");
        const text = useFahrenheit ? "°F" : "°C";
        fadeText(document.getElementById("toggleUnit"), text);
        updateDisplayedTemp();
        updateStatsDisplay(); 
        updateTemperatureStats();
    });
	
});

let useFahrenheit = false;
let latestTempC = null;
let latestTempF = null;


const savedUnit = localStorage.getItem("tempUnit");
if (savedUnit === "F") {
    useFahrenheit = true;
    document.querySelector("#toggleUnit span").textContent = "°F";
} else {
    useFahrenheit = false;
    document.querySelector("#toggleUnit span").textContent = "°C";
}

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
let lastTotal = null;
let lastMax = null;
let lastMin = null;

function updateStatsDisplay() {
    const { unit, max, min } = getUnit();
	
	const stto = `Ilość odczytów - ${statData.total}`;
	const stmx = `Najwyższa temperatura - ${max}${unit}`;
	const stmi = `Najniższa temperatura - ${min}${unit}`;
	const stst = `Pierwszy odczyt - ${statData.start}`;
	
	if (stto !== lastTotal) {
        fadeText(document.getElementById("stat-total"), stto);
        lastTotal = stto;
    }

    if (stmx !== lastMax) {
        fadeText(document.getElementById("stat-max"), stmx);
        lastMax = stmx;
    }

    if (stmi !== lastMin) {
        fadeText(document.getElementById("stat-min"), stmi);
        lastMin = stmi;
    }
	
	document.getElementById("stat-start").textContent = stst;
}

let lastDisplayedTemp = null;

function updateDisplayedTemp() {
	const { temp, unit } = getUnit();
	const newText = temp + unit;

	if (newText === lastDisplayedTemp) return;
	
	lastDisplayedTemp = newText;
	
	fadeText(document.getElementById("temp"), newText);
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

function fadeText(element, newText) {
	if (!element) return;
	
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


function validateDates() {
  const start = document.getElementById('startDate').value;
  const end = document.getElementById('endDate').value;
  if (!start || !end) {
    alert('Proszę podać obie daty.');
    return false;
  }
  if (start > end) {
    alert('Data początkowa nie może być późniejsza niż data końcowa.');
    return false;
  }
  return true;
}
