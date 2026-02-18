async function loadLeaderboard() {
    try {
        const res = await fetch('/api/leaderboard');
        if (!res.ok){
            const txt = await res.text(); 
            throw new Error("Failed to fetch leaderboard: " + txt);
        }
    
        const data = await res.json();
        const tbody = document.querySelector("#leaderboard tbody");
        tbody.innerHTML = '';

        let totalMoves = 0;
        let totalTime = 0;
        let count = 0;

        for (let key in data) {
            const entry = data[key];
            const row = document.createElement('tr');
            row.innerHTML = `
                <td>${entry.Rank}</td>
                <td>${entry.Name}</td>
                <td>${entry.NumOfMoves}</td>
                <td>${entry.Time}</td>
            `;
            tbody.appendChild(row);
            
            var timeArray = entry.Time.split(":");
            var timeSeconds = 
                (parseInt(timeArray[0], 10) * 60 * 60) +
                (parseInt(timeArray[1], 10) * 60) +
                parseInt(timeArray[2], 10);

            totalMoves += entry.NumOfMoves;
            totalTime += timeSeconds;
            count++;
        }

        document.getElementById("avgMoves").textContent = count ? (totalMoves / count).toFixed(2) : 0;
        document.getElementById("avgTime").textContent = count ? (totalTime / count).toFixed(2) : 0;

    } catch (err) {
        console.error(err);
        alert("Error: " + err);
    }
}

loadLeaderboard();