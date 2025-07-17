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

  if (!start || !end) {
    document.getElementById("history-table").innerHTML = "<tr><td colspan='3'>Nie podano zakresu dat.</td></tr>";
    return;
  }

  document.getElementById("range-title").textContent = `Historia temperatur od ${start} do ${end}`;

  document.getElementById("searchInput").addEventListener("input", () => {
    page = 1;
    renderTableFiltered();
  });

  const limitSelect = document.getElementById("limitSelect");
  limitSelect.value = limit;
  limitSelect.addEventListener("change", (e) => {
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
      document.getElementById("pagination").innerHTML = "";
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
    document.getElementById("pagination").innerHTML = "";
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
  const container = document.getElementById("pagination");
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

  container.appendChild(makeButton("« Prev", page - 1, page <= 1));
  container.appendChild(makeButton("First", 1, page <= 1));

  const info = document.createElement("span");
  info.textContent = `Page ${page} of ${totalPages} (Total: ${totalRecords})`;
  container.appendChild(info);

  container.appendChild(makeButton("Last", totalPages, page >= totalPages));
  container.appendChild(makeButton("Next »", page + 1, page >= totalPages));
}
