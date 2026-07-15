let xValues = [];
let yValues = [];
let xValues2 = [];
let yValues2 = [];

let chartDataC = [];
let chartDataF = [];
let chartDataMonthC = [];
let chartDataMonthF = [];

let chartInstance = null;
let useFahrenheit = false;
let latestTempC = null;
let latestTempF = null;
let esp32Date = null;

let statData = {};
let chaData = {};
let socket;
let Bar, Menu;

let isAppReady = false; 
let hasStatsData = false;
let hasChartData = false;
let minimumTimeElapsed = false;

setTimeout(() => {
    minimumTimeElapsed = true;
    hideLoadingScreen();
}, 800); 

function hideLoadingScreen(force = false) {
    if (force || (minimumTimeElapsed && hasStatsData && hasChartData)) {
        isAppReady = true;
        const overlay = document.getElementById('loading-overlay');
        if (overlay) {
            overlay.classList.add('fade-away');
            
            setTimeout(() => {
                const bottomSection = document.querySelector('.Bottom');
                const tempSta = document.querySelector('.TempStat');
                const timeEl = document.querySelector('.Time');
                const tempStatsEl = document.querySelector('.TempTimeStat');
                const tempEl = document.getElementById("temp");

                if (bottomSection) bottomSection.classList.add('ShowOnLoad');
                if (tempSta) tempSta.classList.add('ShowOnLoad');
                if (timeEl) timeEl.classList.add('ShowOnLoad');
                if (tempStatsEl) tempStatsEl.classList.add('ShowOnLoad');
                if (tempEl) tempEl.classList.add('ShowOnLoad');
            }, 100); 
        }
    }
}

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

function toggleCha(title, datay, datax, clickedElement) {
    if (!clickedElement) return;
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

    if (!isAppReady) {
        if (title !== 'Historia') {
            chartsContainer.classList.remove('hidden', 'fade-out-height');
            summarySection.classList.remove('hidden', 'fade-out-height');
            if (searchForm.classList.contains('active')) {
                searchForm.classList.remove('active');
            }
            
            createCha(title, datax, datay);
            updateTemperatureStats();
            
            chartCanvas.style.opacity = '1';
            tempStatsElement.style.opacity = '1';
            timeElement.style.opacity = '1';
        }
        return; 
    }

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
        timeElement.classList.add('fade-out');
        tempStatsElement.classList.add('fade-out');

        setTimeout(() => {
            chartsContainer.classList.remove('hidden');
            summarySection.classList.remove('hidden');
            chartsContainer.classList.remove('fade-out-height');
            summarySection.classList.remove('fade-out-height');
            
            createCha(title, datax, datay);
            updateTemperatureStats();
            
            chartCanvas.style.opacity = '1';
            tempStatsElement.style.opacity = '1';
            timeElement.style.opacity = '1';
            timeElement.classList.remove('fade-out');
            tempStatsElement.classList.remove('fade-out');
        }, transitionDuration - 200);
    }

    if (title === 'Historia') {
        document.getElementById('startDate').value = '';
        document.getElementById('endDate').value = '';
    }
}

function handleGlobalInteraction(e) {
    const isClickOutside = e.type === 'click' && !e.target.closest('.OptionsBar');
    const isEscapeKey = e.type === 'keydown' && e.key === 'Escape';

    if (isClickOutside || isEscapeKey) {
        if (Menu) Menu.classList.remove('show');
        if (Bar) Bar.classList.remove('active');
    }
}

window.addEventListener('DOMContentLoaded', () => { 
    window.scrollBy({ top: -100, behavior: 'smooth' });
    const defaultTab = document.querySelector('.Navbar div.active');

    Bar = document.querySelector('.OptionsBar');
    Menu = document.getElementById('Options');

    if (Bar) {
        Bar.addEventListener('click', function (e) {
            e.stopPropagation();
            Menu.classList.toggle('show');
            Bar.classList.toggle('active');
        });
    }

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
        updateChartForCurrentUnit();
        updateMonthlyChartDisplay();
    });

    const savedUnit = localStorage.getItem("tempUnit");
    if (savedUnit === "F") {
        useFahrenheit = true;
        const toggleSpan = document.querySelector("#toggleUnit span");
        if (toggleSpan) toggleSpan.textContent = "°F";
    } else {
        useFahrenheit = false;
        const toggleSpan = document.querySelector("#toggleUnit span");
        if (toggleSpan) toggleSpan.textContent = "°C";
    }
});

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
                hasChartData = true;
                hideLoadingScreen();
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

    socket.onerror = function(e) {
        console.warn("WebSocket error:", e);
        hideLoadingScreen(true);
    };

    socket.onclose = function() {
        console.warn("WebSocket closed.");
        hideLoadingScreen(true);
    };

} catch (e) {
    console.warn("WebSocket not initialized:", e.message);
    hideLoadingScreen(true);
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

function updateDateFromESP32() {
    if (esp32Date) {
        document.getElementById("data").innerHTML = esp32Date;
        hasStatsData = true;
        hideLoadingScreen();
        return;
    }

    if (statData && statData.start) {
        const startDate = statData.start;
        if (startDate && startDate !== 'unknown' && !startDate.startsWith('1970')) {
            const parts = startDate.split(' ');
            if (parts.length > 0) {
                const dateParts = parts[0].split('-');
                if (dateParts.length === 3) {
                    esp32Date = `${dateParts[2]}.${dateParts[1]}.${dateParts[0]}`;
                    document.getElementById("data").innerHTML = esp32Date;
                    hasStatsData = true;
                    hideLoadingScreen();
                    return;
                }
            }
        }
    }
    
    if (isAppReady) {
        const fallbackDate = new Intl.DateTimeFormat("pl-PL", {
            day: "2-digit", month: "2-digit", year: "numeric"
        }).format(new Date());
        document.getElementById("data").innerHTML = fallbackDate;
    }
}

function updateChartDisplay() {
    if (!chaData || !chaData.data) return;

    const currentHour = new Date().getHours();
    const rolling24Hours = [];
    for (let i = 23; i >= 0; i--) {
        const h = (currentHour - i + 24) % 24;
        rolling24Hours.push(`${h.toString().padStart(2, '0')}:00`);
    }

    const valuesMapC = {};
    const valuesMapF = {};
    chaData.data.forEach(item => {
        const parts = item.hour.split(' ');
        const hour = parts.length > 1 ? parts[1] : item.hour;
        valuesMapC[hour] = item.avg_c;
        valuesMapF[hour] = item.avg_f;
    });

    xValues = rolling24Hours;
    chartDataC = rolling24Hours.map(hour => valuesMapC[hour] !== undefined ? valuesMapC[hour] : null);
    chartDataF = rolling24Hours.map(hour => valuesMapF[hour] !== undefined ? valuesMapF[hour] : null);

    updateChartForCurrentUnit();
}

function updateChartForCurrentUnit() {
    const { unit } = getUnit();
    yValues = useFahrenheit ? chartDataF : chartDataC;

    if (yValues && yValues.length > 0 && yValues.some(val => val !== null)) {
        const activeLabel = document.querySelector('.Navbar .active')?.innerText;
        if (activeLabel === 'Dzisiaj') {
            toggleCha('Dzisiaj', yValues, xValues, document.querySelector('.Navbar .active'));
        }
    }
}

function updateMonthlyChartDisplay() {
    if (!chaData || !chaData.monthly) return;

    const today = new Date();
    const allDays = Array.from({ length: 30 }, (_, i) => {
        const d = new Date(today.getTime() - (29 - i) * 24 * 3600 * 1000);
        return d.toLocaleDateString("pl-PL", { day: "2-digit", month: "2-digit" });
    });

    const valuesMapC = {};
    const valuesMapF = {};
    chaData.monthly.forEach(item => {
        const parts = item.day.split('-');
        const key = `${parts[2]}.${parts[1]}`;
        valuesMapC[key] = item.avg_c;
        valuesMapF[key] = item.avg_f;
    });

    xValues2 = allDays;
    chartDataMonthC = allDays.map(day => valuesMapC[day] !== undefined ? valuesMapC[day] : null);
    chartDataMonthF = allDays.map(day => valuesMapF[day] !== undefined ? valuesMapF[day] : null);

    updateMonthlyChartForCurrentUnit();
}

function updateMonthlyChartForCurrentUnit() {
    const { unit } = getUnit();
    yValues2 = useFahrenheit ? chartDataMonthF : chartDataMonthC;
    
    if (yValues2 && yValues2.length > 0 && yValues2.some(val => val !== null)) {
        const activeLabel = document.querySelector('.Navbar .active')?.innerText;
        if (activeLabel === 'W miesiącu') {
            toggleCha('W miesiącu', yValues2, xValues2, document.querySelector('.Navbar .active'));
        }
    }
}

function updateStatsDisplay() {
    const { unit, max, min } = getUnit();

    const stto = `Ilość odczytów - ${statData.total || 0}`;
    const stmx = `Najwyższa temperatura - ${max !== undefined ? max : '--'}${unit}`;
    const stmi = `Najniższa temperatura - ${min !== undefined ? min : '--'}${unit}`;
    const stst = `Pierwszy odczyt - ${statData.start || '--'}`;

    const totalEl = document.getElementById("stat-total");
    const maxEl = document.getElementById("stat-max");
    const minEl = document.getElementById("stat-min");
    const startEl = document.getElementById("stat-start");

    if (totalEl) fadeText(totalEl, stto);
    if (maxEl) fadeText(maxEl, stmx);
    if (minEl) fadeText(minEl, stmi);
    if (startEl) startEl.textContent = stst;
}

function updateDisplayedTemp() {
    const { temp, unit } = getUnit();
    const newText = (temp !== null ? temp : 'Ładowanie...') + unit;
    const tempEl = document.getElementById("temp");

    if (tempEl) {
        fadeText(tempEl, newText);
    }
}

function updateTemperatureStats() {
    const activeTab = document.querySelector('.Navbar div.active');
    if (!activeTab) return;
    let datay, title;

    if (activeTab.textContent.includes('Dzisiaj')) {
        datay = yValues;
        title = 'Dzisiaj';
    } else if (activeTab.textContent.includes('W miesiącu')) {
        datay = yValues2;
        title = 'W miesiącu';
    } else {
        const timeEl = document.querySelector('.Time');
        const statEl = document.querySelector('.TempTimeStat');
        if (timeEl) timeEl.textContent = '';
        if (statEl) statEl.innerHTML = '';
        return;
    }
		if (!datay || datay.length === 0) {
        return;
    }
    
    datay = datay.filter(function (el) {
        return el != null;
    });

    if (datay.length === 0) return;
    
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
    if (tempTimeStatElement && tempTimeStatElement.innerHTML !== newTempStatsHtml) {
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
        
    if (targetElement.textContent === newText) {
        return;
    }

    if (!isAppReady) {
        targetElement.textContent = newText;
        return;
    }

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

const originalUpdateStatsDisplay = updateStatsDisplay;
updateStatsDisplay = function() {
    originalUpdateStatsDisplay();
    updateDateFromESP32();
};

const originalUpdateChartDisplay = updateChartDisplay;
updateChartDisplay = function() {
    originalUpdateChartDisplay();
    if (chaData && chaData.data && chaData.data.length > 0) {
        const lastEntry = chaData.data[chaData.data.length - 1];
        if (lastEntry && lastEntry.hour && !lastEntry.hour.startsWith('1970')) {
            const parts = lastEntry.hour.split(' ');
            if (parts.length > 0) {
                const dateParts = parts[0].split('-');
                if (dateParts.length === 3) {
                    esp32Date = `${dateParts[2]}.${dateParts[1]}.${dateParts[0]}`;
                    document.getElementById("data").innerHTML = esp32Date;
                    hasStatsData = true;
                    hideLoadingScreen();
                }
            }
        }
    }
};

setTimeout(() => {
    hideLoadingScreen(true);
}, 4000);

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