let start, end;
let page = 1;
let limit = 20;
let totalPages = 1;
let totalRecords = 0;
let allData = [];

document.addEventListener("DOMContentLoaded", () => {
  const params = new URLSearchParams(window.location.search);
  start = params.get("start");
  end = params.get("end");
  
  const table = document.querySelector('table');
  const footer = document.querySelector('footer');
  const controls = document.querySelector('.Controls');


  const time = 50;
    if (table) {
        setTimeout(() => {
            table.classList.add('ShowOnLoad');
        }, time); 
    }
    if (footer) {
        setTimeout(() => {
            footer.classList.add('ShowOnLoad');
        }, time*4); 
    }
    if (controls) {
        setTimeout(() => {
            controls.classList.add('ShowOnLoad');
        }, time*3); 
    }
  if (!start || !end) {
    document.getElementById("history-table").innerHTML = "<tr><td colspan='3'>Nie podano zakresu dat.</td></tr>";
    return;
  }

  document.getElementById("TitleText").textContent = `Historia temperatur od ${start} do ${end}`;

  document.getElementById("searchInput").addEventListener("input", () => {
    page = 1;
    renderTableFiltered();
  });

  const LimitSelect = document.getElementById("LimitSelect");
  LimitSelect.value = limit;
  LimitSelect.addEventListener("change", (e) => {
    limit = parseInt(e.target.value);
    page = 1;
    renderTableFiltered();
  });

  fetchAllData();


});

function fetchAllData() {
  fetch(`/api/history?start=${start}&end=${end}`)
    .then(res => res.json())
    .then(json => {
      allData = json.data || [];
      totalRecords = allData.length;
      page = 1;
      renderTableFiltered();
    })
    .catch(() => {
      document.getElementById("history-table").innerHTML = "<tr><td colspan='3'>Błąd pobierania danych.</td></tr>";
      document.getElementById("Pagination").innerHTML = "";
    });
}

function renderTableFiltered() {
  const search = document.getElementById("searchInput").value.toLowerCase();
  const tbody = document.getElementById("history-table");
  tbody.innerHTML = "";

  const filtered = allData.filter(row =>
    row.timestamp.toLowerCase().includes(search) ||
    row.c.toString().includes(search) ||
    row.f.toString().includes(search)
  );

  totalRecords = filtered.length;
  totalPages = Math.max(1, Math.ceil(totalRecords / limit));

  if (page > totalPages) page = totalPages;
  if (page < 1) page = 1;

  const startIdx = (page - 1) * limit;
  const endIdx = startIdx + limit;
  const paginated = filtered.slice(startIdx, endIdx);

  if (paginated.length === 0) {
    tbody.innerHTML = "<tr><td colspan='3'>Brak wyników pasujących do wyszukiwania.</td></tr>";
    document.getElementById("Pagination").innerHTML = "";
    return;
  }

  for (const row of paginated) {
    const tr = document.createElement("tr");
    tr.innerHTML = `<td>${row.timestamp}</td><td>${parseFloat(row.c).toFixed(2)}</td><td>${parseFloat(row.f).toFixed(2)}</td>`;
    tbody.appendChild(tr);
  }

  renderPagination();
}

function renderPagination() {
  const container = document.getElementById("Pagination");
  container.innerHTML = "";
  container.style.marginTop = "20px";
  container.style.textAlign = "center";

  const makeButton = (label, newPage, disabled) => {
    const btn = document.createElement(disabled ? "span" : "a");
    btn.textContent = label;
    btn.style.margin = "0 10px";
    if (!disabled) {
      btn.href = "javascript:void(0)";
      btn.addEventListener("click", () => {
        page = newPage;
        renderTableFiltered();
      });
    } else {
      btn.style.color = "#aaa";
      btn.style.cursor = "default";
    }
    return btn;
  };  

  container.appendChild(makeButton("« Poprzednia", page - 1, page <= 1));
  container.appendChild(makeButton("Pierwsza", 1, page <= 1));

  const info = document.createElement("span");
  info.textContent = `Strona ${page} z ${totalPages} (Total: ${totalRecords})`;
  container.appendChild(info);

  container.appendChild(makeButton("Ostatnia", totalPages, page >= totalPages));
  container.appendChild(makeButton("Następna »", page + 1, page >= totalPages));
}
