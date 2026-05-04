function update() {
    fetch('/data')
    .then(response => response.json())
    .then(data => {
        const container = document.getElementById("slots");
        container.innerHTML = "";

        data.slots.forEach((status, index) => {
            let div = document.createElement("div");
            let isFree = status === "FREE";
            
            div.className = "slot " + (isFree ? "free" : "occupied");
            div.innerHTML = `SLOT ${index + 1} <span>${isFree ? 'AVAILABLE' : 'OCCUPIED'}</span>`;
            container.appendChild(div);
        });

        document.getElementById("gate").innerHTML = "Gate Status: <strong>" + data.gate + "</strong>";
    })
    .catch(err => console.error("Error fetching data:", err));
}

// Fetch data every 1 second
setInterval(update, 1000);
update();