
let xValues = [];
let yValues = [];

let xValues2 = [];
let yValues2 = [];

let chartInstance = null;

function getIsolatedPoints(data) {
  return data.map((value, index) => {
    if (value === null) return false;
    const prev = data[index - 1];
    const next = data[index + 1];
    return (prev === null || prev === undefined) && 
           (next === null || next === undefined);
  });
}

function createCha(title, datax, datay) {
    const ctx = document.getElementById('TempCha').getContext('2d');
	
	const isolatedPoints = getIsolatedPoints(datay);
	const pointRadius = isolatedPoints.map(isolated => isolated ? 2 : 0);
	const { unit } = getUnit();

    if (chartInstance) {
        chartInstance.destroy();
    }

    chartInstance = new Chart(ctx, {
        type: 'line',
        data: {
            labels: datax,
            datasets: [{
                label: 'Temperatura (' + unit + ')',
                data: datay,
                borderColor: 'rgba(209, 78, 42, 1)',
                backgroundColor: 'rgba(209, 78, 42, 1)',
                tension: 0.5,
                fill: false,
                pointRadius: pointRadius,
				pointHitRadius: 5,
				pointStyle: 'rect'
            }]
        },
        options: {
            maintainAspectRatio: false,
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
				
				tooltip: {
					titleAlign: 'center',
					callbacks: {
						title: function(context) {
							return `${context[0].label}`;
						},
						label: function(context) {
							return `${context.parsed.y}${unit}`;
						}
					}
				},

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
    const searchForm = document.getElementById('SearchHistory');
    const chartsContainer = document.querySelector('.Charts');
    const summarySection = document.querySelector('.Summary');

    const transitionDuration = 300;
    const isSwitchingFromHistoria = searchForm.classList.contains('active');

    if (title === 'Historia') {
        chartsContainer.classList.add('fade-out-height');
        summarySection.classList.add('fade-out-height');

        setTimeout(() => {
            chartsContainer.classList.add('hidden');
            summarySection.classList.add('hidden');
            searchForm.classList.add('active');
        }, transitionDuration);
    } else {
    if (isSwitchingFromHistoria && window.scrollY > 150) {
        window.scrollBy({ top: -100, behavior: 'smooth' });
    }

    if (searchForm.classList.contains('active')) {
        searchForm.classList.remove('active');
    }

    chartCanvas.style.opacity = '0';
    timeElement.style.opacity = '0';
    tempStatsElement.style.opacity = '0';

    setTimeout(() => {
        chartsContainer.classList.remove('hidden');
        summarySection.classList.remove('hidden');
        chartsContainer.classList.remove('fade-out-height');
        summarySection.classList.remove('fade-out-height');

        if (firstLoad) {
            createCha(title, datax, datay);
            updateTemperatureStats();
            chartCanvas.style.opacity = '1';
            tempStatsElement.style.opacity = '1';
            timeElement.style.opacity = '1';
            firstLoad = false;
        } else {
            timeElement.classList.add('fade-out');
            tempStatsElement.classList.add('fade-out');

            setTimeout(() => {
                createCha(title, datax, datay);
                updateTemperatureStats();
                chartCanvas.style.opacity = '1';
                tempStatsElement.style.opacity = '1';
                timeElement.style.opacity = '1';
                timeElement.classList.remove('fade-out');
                tempStatsElement.classList.remove('fade-out');
            }, transitionDuration);
        }
    }, transitionDuration);
}


    if (title === 'Historia') {
        document.getElementById('startDate').value = '';
        document.getElementById('endDate').value = '';
    }
}
setTimeout(() => {
  chartCanvas.style.opacity = '1';
}, 50); 




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
	window.scrollBy({ top: -100, behavior: 'smooth' })
    const defaultTab = document.querySelector('.Navbar div.active');
    toggleCha('Dzisiaj', yValues, xValues, defaultTab);

    const time = 50;

    Bar = document.querySelector('.OptionsBar');
    Menu = document.getElementById('Options');

    Bar.addEventListener('click', function (e) {
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
        fadeText(document.getElementById("toggleUnit").querySelector('span'), text);
        updateDisplayedTemp();
        updateStatsDisplay();
        updateTemperatureStats();
		updateChartForCurrentUnit();
		updateMonthlyChartDisplay();
		UpdateChartLegend();
    });

    const savedUnit = localStorage.getItem("tempUnit");
    if (savedUnit === "F") {
        useFahrenheit = true;
        document.querySelector("#toggleUnit span").textContent = "°F";
    } else {
        useFahrenheit = false;
        document.querySelector("#toggleUnit span").textContent = "°C";
    }

    const bottomSection = document.querySelector('.Bottom');
    const TempSta = document.querySelector('.TempStat');
    const Title = document.querySelector('.Title');

    if (bottomSection) {
        setTimeout(() => {
            bottomSection.classList.add('ShowOnLoad');
        }, time); 
    }
    
    if (TempSta) {
        setTimeout(() => {
            TempSta.classList.add('ShowOnLoad');
        }, time + 100); 
    }
});

let useFahrenheit = false;
let latestTempC = null;
let latestTempF = null;


let statData = {};
let chaData = {};
let socket;

try {
    const wsUrl = 'ws://' + location.hostname + ':81/';

    if (!location.hostname) {
        throw new Error("Invalid hostname for WebSocket.");
    }

    socket = new WebSocket(wsUrl);

    socket.onmessage = function (event) {
        try {
            const data = JSON.parse(event.data);
			if (data.type === "chart") {
				chaData = data;
				updateChartDisplay();
				updateMonthlyChartDisplay();

			}
            else if (data.total !== undefined) {
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

let chartDataC = [];
let chartDataF = [];

function updateChartDisplay() {
    if (!chaData || !chaData.data) return;

    const now = new Date();
    const hours = Array.from({ length: 24 }, (_, i) => {
        const d = new Date(now.getTime() - (23 - i) * 3600 * 1000);
        return `${d.getHours().toString().padStart(2, '0')}:00`;
    });

    const valuesMapC = {};
    const valuesMapF = {};

    chaData.data.forEach(item => {
        const date = new Date(item.hour);
        const hourKey = `${date.getHours().toString().padStart(2, '0')}:00`;
        valuesMapC[hourKey] = item.avg_c;
        valuesMapF[hourKey] = item.avg_f;
    });

    xValues = hours;
    chartDataC = hours.map(h => valuesMapC[h] ?? null);
    chartDataF = hours.map(h => valuesMapF[h] ?? null);

    updateChartForCurrentUnit();
}

function updateChartForCurrentUnit() {
    const { unit } = getUnit();
    yValues = useFahrenheit ? chartDataF : chartDataC;

    const activeLabel = document.querySelector('.Navbar .active')?.innerText;
    if (activeLabel === 'Dzisiaj') {
        toggleCha('Dzisiaj', yValues, xValues, document.querySelector('.Navbar .active'));
    }
}

function updateMonthlyChartDisplay() {
    if (!chaData || !chaData.monthly) return;

    const today = new Date();
    const days = Array.from({ length: 30 }, (_, i) => {
		const d = new Date(today.getTime() - (29 - i) * 24 * 3600 * 1000);
		return d.toLocaleDateString("pl-PL", { day: "2-digit", month: "2-digit" }); // Gives "DD-MM"
	});

    const valuesMapC = {};
    const valuesMapF = {};

    chaData.monthly.forEach(item => {
        const [year, month, day] = item.day.split('-'); // "YYYY-MM-DD"
        const key = `${day}.${month}`; // Convert to "DD-MM"
        valuesMapC[key] = item.avg_c;
        valuesMapF[key] = item.avg_f;
    });

    xValues2 = days;
    chartDataMonthC = days.map(day => valuesMapC[day] ?? null);
    chartDataMonthF = days.map(day => valuesMapF[day] ?? null);

    updateMonthlyChartForCurrentUnit();
}

function updateMonthlyChartForCurrentUnit() {
    const { unit } = getUnit();
    yValues2 = useFahrenheit ? chartDataMonthF : chartDataMonthC;
	
    const activeLabel = document.querySelector('.Navbar .active')?.innerText;
	if (activeLabel === 'W miesiącu') {
		toggleCha('W miesiącu', yValues2, xValues2, document.querySelector('.Navbar .active'));
	}
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

    if (document.getElementById("stat-total").textContent !== stto) {
        fadeText(document.getElementById("stat-total").querySelector('span'), stto);
    }
    if (document.getElementById("stat-max").textContent !== stmx) {
        fadeText(document.getElementById("stat-max").querySelector('span'), stmx);
    }
    if (document.getElementById("stat-min").textContent !== stmi) {
        fadeText(document.getElementById("stat-min").querySelector('span'), stmi);
    }
    document.getElementById("stat-start").textContent = stst;
}

let lastDisplayedTemp = null;

function updateDisplayedTemp() {
    const { temp, unit } = getUnit();
    const newText = (temp !== null ? temp : 'Ładowanie...') + unit;

    if (document.getElementById("temp").textContent !== newText) {
        fadeText(document.getElementById("temp").querySelector('span'), newText);
    }
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
    } else {
        document.querySelector('.Time').textContent = '';
        document.querySelector('.TempTimeStat').innerHTML = '';
        return;
    }

    const minTemp = Math.min(...datay);
    const maxTemp = Math.max(...datay);
    const avgTemp = (datay.reduce((a, b) => a + b, 0) / datay.length).toFixed(2);
    const unit = useFahrenheit ? '°F' : '°C';

    fadeText(document.querySelector('.Time'), title.toUpperCase());

    const newTempStatsHtml =
        `TEMPERATURA<br><span class="red">MIN: </span>${minTemp}${unit} | ` +
        `<span class="red">ŚRD: </span>${avgTemp}${unit} | ` +
        `<span class="red">MAKS:</span> ${maxTemp}${unit}`;

    const tempTimeStatElement = document.querySelector('.TempTimeStat');
    if (tempTimeStatElement.innerHTML !== newTempStatsHtml) {
        tempTimeStatElement.classList.add('fade-out');
        setTimeout(() => {
            tempTimeStatElement.innerHTML = newTempStatsHtml;
            tempTimeStatElement.classList.remove('fade-out');
            tempTimeStatElement.classList.add('fade-in');
            setTimeout(() => {
                tempTimeStatElement.classList.remove('fade-in');
            }, 300);
        }, 300);
    }
}

function fadeText(element, newText) {
    if (!element) return;

    const targetElement = element.querySelector('span') || element;

    targetElement.classList.remove('fade-in');
    targetElement.classList.add('fade-out');

    setTimeout(() => {
        targetElement.textContent = newText;
        targetElement.classList.remove('fade-out');
        targetElement.classList.add('fade-in');

        setTimeout(() => {
            targetElement.classList.remove('fade-in');
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
function submitSearch() {
    if (!validateDates()) return;

    const start = document.getElementById('startDate').value;
    const end = document.getElementById('endDate').value;

    window.location.href = `/history.html?start=${start}&end=${end}`;
}