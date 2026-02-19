import { formatTime } from "./utils.js";

const gridEl = document.getElementById("grid");
const movesEl = document.getElementById("moves");
const timeEl  = document.getElementById("time");
const statusEl = document.getElementById("status");
const newBtn = document.getElementById("newGameBtn");

const nameEl = document.getElementById("playerName");
nameEl.value = (localStorage.getItem("playerName3") || "").toUpperCase();

// Server state (produced by C++ PuzzleLogic.h)
let state = { board: [], validMoves: [], moves: 0, solved: false };

// Timer + submit control
let startTimeMs = 0;
let timerId = null;
let submitted = false;

function startTimer() {
    stopTimer();
    startTimeMs = Date.now();
    timeEl.textContent = "00:00:00";
    timerId = setInterval(() => {
      timeEl.textContent = formatTime(elapsedSeconds());
    }, 250);
}

function stopTimer() {
    if (timerId) clearInterval(timerId);
    timerId = null;
}

function elapsedSeconds() {
    return Math.floor((Date.now() - startTimeMs) / 1000);
}

function getName3() {
    const cleaned = (nameEl.value || "")
      .toUpperCase()
      .replace(/[^A-Z]/g, "")
      .slice(0, 3);

    // keep the input clean as they type
    nameEl.value = cleaned;

    if (cleaned.length !== 3) return null;

    localStorage.setItem("playerName3", cleaned);
    return cleaned;
}

async function submitScore() {
    const name = getName3();
    if (!name) {
      statusEl.textContent = "Solved! Enter a 3-letter name to submit.";
      submitted = false; // allow retry
      return;
    }

    const res = await fetch("/api/submit", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        name,
        moves: state.moves ?? 0,
        timeSeconds: elapsedSeconds()
      })
    });

    if (res.ok) {
      statusEl.textContent = "Solved! Score submitted ✅";
    } else {
      const txt = await res.text();
      statusEl.textContent = "Solved! Submit failed: " + txt;
      submitted = false; // allow retry
    }
}

function render() {
    // save old positions BEFORE clearing grid
    const oldPos = {};
    document.querySelectorAll(".cell").forEach(c => {
      oldPos[c.textContent] = c.getBoundingClientRect();
    });

    gridEl.innerHTML = "";
    movesEl.textContent = state.moves ?? 0;
    statusEl.textContent = state.solved ? "Solved!" : "";

    (state.board || []).forEach((val, idx) => {
      const cell = document.createElement("button");
      cell.className = "cell";

      if (val === 0) {
        cell.classList.add("empty");
        cell.disabled = true;
        gridEl.appendChild(cell);
        return;
      }

      cell.textContent = val;

      const canMove = (state.validMoves || []).includes(idx);
      cell.disabled = !canMove || state.solved;
      if (canMove) cell.classList.add("movable");

      cell.addEventListener("click", () => makeMove(idx));
      gridEl.appendChild(cell);

      // animation magic
      const newBox = cell.getBoundingClientRect();
      const oldBox = oldPos[val];

      if (oldBox) {
        const dx = oldBox.left - newBox.left;
        const dy = oldBox.top - newBox.top;

        cell.style.transform = `translate(${dx}px, ${dy}px)`;
        cell.style.transition = "none";

        requestAnimationFrame(() => {
          cell.style.transform = "";
          cell.style.transition = "transform 0.50s cubic-bezier(.2,.8,.2,1)";
        });
      }
    });
}

async function newGame() {
    statusEl.textContent = "Loading...";
    submitted = false;
    startTimer();

    const res = await fetch("/api/new");
    if (!res.ok) {
      statusEl.textContent = "Missing /api/new on server";
      stopTimer();
      return;
    }
    state = await res.json();
    render();
}

async function makeMove(pos) {
    const res = await fetch("/api/move", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ pos })
    });

    if (!res.ok) {
      statusEl.textContent = "Missing /api/move on server";
      return;
    }

    state = await res.json();
    render();

    // submit ONCE when solved
    if (state.solved && !submitted) {
      submitted = true;
      stopTimer();
      submitScore();
    }
}

newBtn.addEventListener("click", newGame);
newGame();