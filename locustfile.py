from locust import HttpUser, task, between, LoadTestShape
import random
import json


class StepLoadShape(LoadTestShape):
    
    step_time = 120        # seconds at each step (2 minutes)
    step_load = 10         # users to add per step
    spawn_rate = 10        # how fast to add users at each step
    time_limit = 5400      # total test duration in seconds (100 min)

    def tick(self):
        run_time = self.get_run_time()
        
        if run_time > self.time_limit:
            return None  # stop the test
        
        current_step = run_time // self.step_time
        target_users = min(current_step * self.step_load, 100)
        
        return (target_users, self.spawn_rate)


class SuperSorter3400User(HttpUser):
    wait_time = between(1, 3)  # SRS assumes ~1 update/second per user

    def on_start(self):
        """Start a game when a user spawns"""
        self.client.get("/api/startgame")

    # --- Page loads ---

    @task(2)
    def load_home(self):
        """PRF-0080: GET home page, target <= 1s"""
        self.client.get("/")

    @task(2)
    def load_leaderboard_page(self):
        """PRF-0020: GET leaderboard page"""
        self.client.get("/leaderboard")

    @task(1)
    def load_game_page(self):
        """PRF-0010: GET game page, target <= 2s"""
        self.client.get("/game")

    # --- API routes ---

    @task(10)
    def make_move(self):
        """PRF-0070: PATCH move, target <= 200ms — highest frequency, simulates gameplay"""
        pos = random.randint(0, 8)
        self.client.patch("/api/move", json={"pos": pos})

    @task(3)
    def undo_move(self):
        """PRF-0072: PUT undo, target <= 200ms"""
        self.client.put("/api/undo")

    @task(2)
    def get_hint(self):
        """PRF-0071: GET hint, target <= 500ms"""
        self.client.get("/api/hint")

    @task(2)
    def new_game(self):
        """PRF-0073: DELETE new game, target <= 150ms"""
        self.client.delete("/api/new")

    @task(3)
    def get_leaderboard(self):
        """PRF-0020: GET leaderboard data, target <= 1s"""
        self.client.get("/api/leaderboard")

    @task(1)
    def submit_score(self):
        """PRF-0030: POST submit score, target <= 500ms"""
        names = ["AAA", "BBB", "CCC", "DDD", "EEE", "ZZZ"]
        self.client.post("/api/submit", json={
            "name": random.choice(names),
            "moves": random.randint(10, 100),
            "timeSeconds": random.randint(30, 300)
        })