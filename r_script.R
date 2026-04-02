# ============================================================
# packages
# ============================================================
#install.packages("ggplot2")
#install.packages("tidyr")

library(ggplot2)
library(tidyr)

#setwd("C:/Users/lexie/OneDrive/Documents/School/Classes/Software Quality IV/Project/Project6SuperSorter3400")

# ============================================================
# load data
# ============================================================
# test 1 - gradual ramp up
locust_1 <- read.csv("1_locust_requests.csv")
stats_1_full <- read.csv("1_stats.csv")
head(locust_1)
head(stats_1_full)

# test 2 - step ramp up
locust_2 <- read.csv("2_locust_requests.csv")
stats_2_full <- read.csv("2_stats.csv")
history_2 <- read.csv("2_locust_stats_history.csv")
head(locust_2)
head(stats_2_full)
head(history_2)

# test 3 - step ramp up with full history
locust_3 <- read.csv("3_locust_requests.csv")
stats_3_full <- read.csv("3_stats.csv")
history_3 <- read.csv("3_locust_requests_full_history.csv")
head(locust_3)
head(stats_3_full)
head(history_3)

# ============================================================
# data prep - test 1
# ============================================================
# remove aggregated row
locust_1_clean <- locust_1[locust_1$Name != "",]

# clean CPU and memory, parse time
stats_1_full$CPU_clean <- as.numeric(gsub("%", "", stats_1_full$CPU))
stats_1_full$Memory_clean <- as.numeric(gsub("MiB.*", "", stats_1_full$Memory))
stats_1_full$Time_parsed <- as.POSIXct(stats_1_full$Time, format="%H:%M:%S")

# trim to first user spawn
stats_1 <- stats_1_full[stats_1_full$Time_parsed >= as.POSIXct("19:13:00", format="%H:%M:%S"),]
stats_1$Time_parsed <- as.POSIXct(stats_1$Time, format="%H:%M:%S")
stats_1$Elapsed_min <- as.numeric(difftime(stats_1$Time_parsed, stats_1$Time_parsed[1], units="mins"))
stats_1$Interval <- floor(stats_1$Elapsed_min / 0.25) * 0.25

# ============================================================
# data prep - test 2
# ============================================================
# remove aggregated row
locust_2_clean <- locust_2[locust_2$Name != "",]

# clean CPU and memory, parse time
stats_2_full$CPU_clean <- as.numeric(gsub("%", "", stats_2_full$CPU))
stats_2_full$Memory_clean <- as.numeric(gsub("MiB.*", "", stats_2_full$Memory))
stats_2_full$Time_parsed <- as.POSIXct(stats_2_full$Time, format="%H:%M:%S")

# trim to 2 minutes before first user spawn
stats_2 <- stats_2_full[stats_2_full$Time_parsed >= as.POSIXct("01:13:00", format="%H:%M:%S"),]
stats_2$Time_parsed <- as.POSIXct(stats_2$Time, format="%H:%M:%S")
stats_2$Elapsed_min <- as.numeric(difftime(stats_2$Time_parsed, stats_2$Time_parsed[1], units="mins"))
stats_2$Interval <- floor(stats_2$Elapsed_min / 0.25) * 0.25

# clean stats history
history_2$Time_parsed <- as.POSIXct(history_2$Timestamp, origin="1970-01-01", tz="UTC")
history_2_active <- history_2[history_2$User.Count > 0,]
history_2_active$Elapsed_min <- as.numeric(difftime(history_2_active$Time_parsed, history_2_active$Time_parsed[1], units="mins"))
history_2_active$Interval <- floor(history_2_active$Elapsed_min / 5) * 5

# ============================================================
# data prep - test 3
# ============================================================
# remove aggregated row
locust_3_clean <- locust_3[locust_3$Name != "",]

# clean CPU and memory, parse time
stats_3_full$CPU_clean <- as.numeric(gsub("%", "", stats_3_full$CPU))
stats_3_full$Memory_clean <- as.numeric(gsub("MiB.*", "", stats_3_full$Memory))
stats_3_full$Time_parsed <- as.POSIXct(stats_3_full$Time, format="%H:%M:%S")

# trim to 2 minutes before first user spawn
stats_3 <- stats_3_full[stats_3_full$Time_parsed >= as.POSIXct("03:19:00", format="%H:%M:%S"),]
stats_3$Time_parsed <- as.POSIXct(stats_3$Time, format="%H:%M:%S")
stats_3$Elapsed_min <- as.numeric(difftime(stats_3$Time_parsed, stats_3$Time_parsed[1], units="mins"))
stats_3$Interval <- floor(stats_3$Elapsed_min / 0.25) * 0.25

# clean full history
history_3$Time_parsed <- as.POSIXct(history_3$Timestamp, origin="1970-01-01", tz="UTC")
history_3_active <- history_3[history_3$User.Count > 0,]
history_3_active$Elapsed_min <- as.numeric(difftime(history_3_active$Time_parsed, history_3_active$Time_parsed[1], units="mins"))
history_3_active$Interval <- floor(history_3_active$Elapsed_min / 5) * 5

# ============================================================
# PRF-0010: game board loading <= 2000ms - test 3
# ============================================================
# filter for game endpoints
history_3_game <- history_3_active[history_3_active$Name %in% c("/game", "/api/startgame"),]

# average response time per endpoint per 5 minute interval
prf_0010 <- aggregate(Total.Average.Response.Time ~ Interval + Name, 
                      data=history_3_game, FUN=mean)

# check how many intervals meet <= 2000ms per endpoint
for(ep in unique(prf_0010$Name)) {
  ep_data <- prf_0010[prf_0010$Name == ep,]
  passing <- sum(ep_data$Total.Average.Response.Time <= 2000)
  total <- nrow(ep_data)
  cat("PRF-0010", ep, ":", passing, "out of", total, "intervals meet <= 2000ms\n")
}

# plot
ggplot(prf_0010, aes(x=Interval, y=Total.Average.Response.Time, color=Name)) +
  geom_line(linewidth=0.85) +
  geom_hline(yintercept=2000, color="red", linetype="dashed", linewidth=1) +
  scale_color_manual(values=c("/game"="steelblue", "/api/startgame"="darkorange")) +
  labs(title="PRF-0010: Game Board Loading Response Time",
       x="Elapsed Time (minutes)", y="Average Response Time (ms)", color="Endpoint") +
  theme_minimal()

# ============================================================
# PRF-0020: leaderboard loading <= 1000ms - test 3
# ============================================================
# filter for leaderboard endpoints
history_3_leaderboard <- history_3_active[history_3_active$Name %in% c("/api/leaderboard", "/leaderboard"),]

# average response time per endpoint per 5 minute interval
prf_0020 <- aggregate(Total.Average.Response.Time ~ Interval + Name,
                      data=history_3_leaderboard, FUN=mean)

# clean up endpoint labels
prf_0020$Label <- ifelse(prf_0020$Name == "/api/leaderboard",
                         "Database Response\n(/api/leaderboard)",
                         "Page Response\n(/leaderboard)")

# check how many intervals meet <= 1000ms per endpoint
for(ep in unique(prf_0020$Name)) {
  ep_data <- prf_0020[prf_0020$Name == ep,]
  passing <- sum(ep_data$Total.Average.Response.Time <= 1000)
  total <- nrow(ep_data)
  cat("PRF-0020", ep, ":", passing, "out of", total, "intervals meet <= 1000ms\n")
}

# plot
ggplot(prf_0020, aes(x=Interval, y=Total.Average.Response.Time, color=Label)) +
  geom_line(linewidth=0.85) +
  geom_hline(yintercept=1000, color="red", linetype="dashed", linewidth=1) +
  scale_color_manual(values=c("Database Response\n(/api/leaderboard)"="steelblue",
                              "Page Response\n(/leaderboard)"="darkorange")) +
  labs(title="PRF-0020: Leaderboard Loading Response Time",
       x="Elapsed Time (minutes)", y="Average Response Time (ms)", color="Endpoint") +
  theme_minimal()

# ============================================================
# PRF-0030: score submission <= 500ms - test 3
# ============================================================
# filter for submit endpoint
history_3_submit <- history_3_active[history_3_active$Name == "/api/submit",]

# average response time per 5 minute interval
prf_0030 <- aggregate(Total.Average.Response.Time ~ Interval,
                      data=history_3_submit, FUN=mean)

# check how many intervals meet <= 500ms
passing_0030 <- sum(prf_0030$Total.Average.Response.Time <= 500)
total_0030 <- nrow(prf_0030)
cat("PRF-0030 /api/submit:", passing_0030, "out of", total_0030, "intervals meet <= 500ms\n")

# plot
ggplot(prf_0030, aes(x=Interval, y=Total.Average.Response.Time)) +
  geom_line(color="steelblue", linewidth=0.85) +
  geom_hline(yintercept=500, color="red", linetype="dashed", linewidth=1) +
  labs(title="PRF-0030: Score Submission Response Time",
       x="Elapsed Time (minutes)", y="Average Response Time (ms)") +
  theme_minimal()

# ============================================================
# PRF-0040: 100 concurrent users - test 1 (gradual ramp)
# ============================================================
elapsed_1 <- seq(0, 80, by=0.083)
users_1 <- pmin(floor(elapsed_1 / 20 * 100), 100)
users_1[1] <- 0
ramp_data_1 <- data.frame(Elapsed=elapsed_1, Users=users_1)

ggplot(ramp_data_1, aes(x=Elapsed, y=Users)) +
  geom_line(color="steelblue", linewidth=0.85) +
  geom_hline(yintercept=100, color="red", linetype="dashed", linewidth=1) +
  labs(title="Test 1: Simulated Users Over Time (Gradual Ramp)",
       x="Elapsed Time (minutes)", y="Number of Users") +
  theme_minimal()

# ============================================================
# PRF-0040: 100 concurrent users - test 2 (step ramp)
# ============================================================
elapsed_2 <- seq(0, 100, by=0.083)
users_2 <- pmin(floor(elapsed_2 / 2) * 10, 100)
users_2[elapsed_2 == 0] <- 0
ramp_data_2 <- data.frame(Elapsed=elapsed_2, Users=users_2)

ggplot(ramp_data_2, aes(x=Elapsed, y=Users)) +
  geom_line(color="steelblue", linewidth=0.85) +
  geom_hline(yintercept=100, color="red", linetype="dashed", linewidth=1) +
  labs(title="Test 2: Simulated Users Over Time (Step Ramp)",
       x="Elapsed Time (minutes)", y="Number of Users") +
  theme_minimal()

# ============================================================
# PRF-0040: 100 concurrent users - test 3 (step ramp with full history)
# ============================================================
ggplot(ramp_data_2, aes(x=Elapsed, y=Users)) +
  geom_line(color="steelblue", linewidth=0.85) +
  geom_hline(yintercept=100, color="red", linetype="dashed", linewidth=1) +
  labs(title="Test 3: Simulated Users Over Time (Step Ramp)",
       x="Elapsed Time (minutes)", y="Number of Users") +
  theme_minimal()

# ============================================================
# PRF-0040: combined user ramp
# ============================================================
ramp_combined <- data.frame(
  Elapsed = c(elapsed_1, elapsed_2),
  Users = c(users_1, users_2),
  Test = c(rep("Test 1 - Gradual Ramp", length(elapsed_1)),
           rep("Tests 2 & 3 - Step Ramp", length(elapsed_2)))
)

ggplot(ramp_combined, aes(x=Elapsed, y=Users, color=Test)) +
  geom_line(linewidth=0.85) +
  geom_hline(yintercept=100, color="black", linetype="dashed", linewidth=1) +
  scale_color_manual(values=c("Test 1 - Gradual Ramp"="steelblue",
                              "Tests 2 & 3 - Step Ramp"="darkorange")) +
  labs(title="Simulated Users Over Time",
       x="Elapsed Time (minutes)", y="Number of Users", color="Test") +
  theme_minimal()

# ============================================================
# memory usage - test 1
# ============================================================
stats_1_mem <- aggregate(Memory_clean ~ Interval, data=stats_1, FUN=mean)

ggplot(stats_1_mem, aes(x=Interval, y=Memory_clean)) +
  geom_line(color="steelblue", linewidth=0.85) +
  ylim(0, 300) +
  labs(title="Test 1: Memory Usage During Load Test",
       x="Elapsed Time (minutes)", y="Memory (MiB)") +
  theme_minimal()

# ============================================================
# memory usage - test 2
# ============================================================
stats_2_mem <- aggregate(Memory_clean ~ Interval, data=stats_2, FUN=mean)

ggplot(stats_2_mem, aes(x=Interval, y=Memory_clean)) +
  geom_line(color="darkorange", linewidth=0.85) +
  ylim(0, 200) +
  labs(title="Test 2: Memory Usage During Load Test",
       x="Elapsed Time (minutes)", y="Memory (MiB)") +
  theme_minimal()

# ============================================================
# memory usage - test 3
# ============================================================
stats_3_mem <- aggregate(Memory_clean ~ Interval, data=stats_3, FUN=mean)

ggplot(stats_3_mem, aes(x=Interval, y=Memory_clean)) +
  geom_line(color="forestgreen", linewidth=0.85) +
  ylim(0, 200) +
  labs(title="Test 3: Memory Usage During Load Test",
       x="Elapsed Time (minutes)", y="Memory (MiB)") +
  theme_minimal()

# ============================================================
# memory usage - combined
# ============================================================
stats_1_mem$Test <- "Test 1 - Gradual"
stats_2_mem$Test <- "Test 2 - Step"
stats_3_mem$Test <- "Test 3 - Step (Full History)"
mem_combined <- rbind(stats_1_mem, stats_2_mem, stats_3_mem)

ggplot(mem_combined, aes(x=Interval, y=Memory_clean, color=Test)) +
  geom_line(linewidth=0.85) +
  ylim(0, 200) +
  scale_color_manual(values=c("Test 1 - Gradual"="steelblue",
                              "Test 2 - Step"="darkorange",
                              "Test 3 - Step (Full History)"="forestgreen")) +
  labs(title="Memory Usage During Load Test",
       x="Elapsed Time (minutes)", y="Memory (MiB)", color="Test") +
  theme_minimal()

# ============================================================
# PRF-0050: CPU utilization <= 80% - test 1
# ============================================================
stats_1_cpu <- aggregate(CPU_clean ~ Interval, data=stats_1, FUN=mean)

passing_cpu_1 <- sum(stats_1_cpu$CPU_clean <= 80)
total_cpu_1 <- nrow(stats_1_cpu)
cat("PRF-0050 Test 1:", passing_cpu_1, "out of", total_cpu_1, "intervals meet <= 80% CPU\n")

ggplot(stats_1_cpu, aes(x=Interval, y=CPU_clean)) +
  geom_line(color="steelblue", linewidth=0.85) +
  geom_hline(yintercept=80, color="red", linetype="dashed", linewidth=1) +
  labs(title="Test 1: CPU Utilization per 15 Seconds",
       x="Elapsed Time (minutes)", y="CPU (%)") +
  theme_minimal()

# ============================================================
# PRF-0050: CPU utilization <= 80% - test 2
# ============================================================
stats_2_cpu <- aggregate(CPU_clean ~ Interval, data=stats_2, FUN=mean)

passing_cpu_2 <- sum(stats_2_cpu$CPU_clean <= 80)
total_cpu_2 <- nrow(stats_2_cpu)
cat("PRF-0050 Test 2:", passing_cpu_2, "out of", total_cpu_2, "intervals meet <= 80% CPU\n")

ggplot(stats_2_cpu, aes(x=Interval, y=CPU_clean)) +
  geom_line(color="steelblue", linewidth=0.85) +
  geom_hline(yintercept=80, color="red", linetype="dashed", linewidth=1) +
  labs(title="Test 2: CPU Utilization per 15 Seconds",
       x="Elapsed Time (minutes)", y="CPU (%)") +
  theme_minimal()

# ============================================================
# PRF-0050: CPU utilization <= 80% - test 3
# ============================================================
stats_3_cpu <- aggregate(CPU_clean ~ Interval, data=stats_3, FUN=mean)

passing_cpu_3 <- sum(stats_3_cpu$CPU_clean <= 80)
total_cpu_3 <- nrow(stats_3_cpu)
cat("PRF-0050 Test 3:", passing_cpu_3, "out of", total_cpu_3, "intervals meet <= 80% CPU\n")

ggplot(stats_3_cpu, aes(x=Interval, y=CPU_clean)) +
  geom_line(color="steelblue", linewidth=0.85) +
  geom_hline(yintercept=80, color="red", linetype="dashed", linewidth=1) +
  labs(title="Test 3: CPU Utilization per 15 Seconds",
       x="Elapsed Time (minutes)", y="CPU (%)") +
  theme_minimal()

# ============================================================
# PRF-0050: CPU utilization - combined
# ============================================================
stats_1_cpu$Test <- "Test 1 - Gradual"
stats_2_cpu$Test <- "Test 2 - Step"
stats_3_cpu$Test <- "Test 3 - Step (Full History)"
cpu_combined <- rbind(stats_1_cpu, stats_2_cpu, stats_3_cpu)

ggplot(cpu_combined, aes(x=Interval, y=CPU_clean, color=Test)) +
  geom_line(linewidth=0.85) +
  geom_hline(yintercept=80, color="black", linetype="dashed", linewidth=1) +
  scale_color_manual(values=c("Test 1 - Gradual"="steelblue",
                              "Test 2 - Step"="darkorange",
                              "Test 3 - Step (Full History)"="forestgreen")) +
  labs(title="CPU Utilization During Load Test",
       x="Elapsed Time (minutes)", y="CPU (%)", color="Test") +
  theme_minimal()

# ============================================================
# PRF-0060: throughput >= 50 req/s - test 2
# ============================================================
throughput_2_5min <- aggregate(Requests.s ~ Interval, data=history_2_active, FUN=mean)

passing_tp_2 <- sum(throughput_2_5min$Requests.s >= 50)
total_tp_2 <- nrow(throughput_2_5min)
cat("PRF-0060 Test 2:", passing_tp_2, "out of", total_tp_2, "intervals meet >= 50 req/s\n")

ggplot(throughput_2_5min, aes(x=Interval, y=Requests.s)) +
  geom_line(color="darkorange", linewidth=1.5) +
  geom_hline(yintercept=50, color="red", linetype="dashed", linewidth=1) +
  labs(title="Test 2: Average Throughput per 5 Minute Interval",
       x="Elapsed Time (minutes)", y="Requests per Second") +
  theme_minimal()

# ============================================================
# PRF-0060: throughput >= 50 req/s - test 3
# ============================================================
# use aggregated rows only for throughput
history_3_agg <- history_3_active[history_3_active$Name == "Aggregated",]
throughput_3_5min <- aggregate(Requests.s ~ Interval, data=history_3_agg, FUN=mean)

passing_tp_3 <- sum(throughput_3_5min$Requests.s >= 50)
total_tp_3 <- nrow(throughput_3_5min)
cat("PRF-0060 Test 3:", passing_tp_3, "out of", total_tp_3, "intervals meet >= 50 req/s\n")

ggplot(throughput_3_5min, aes(x=Interval, y=Requests.s)) +
  geom_line(color="forestgreen", linewidth=1.5) +
  geom_hline(yintercept=50, color="red", linetype="dashed", linewidth=1) +
  labs(title="Test 3: Average Throughput per 5 Minute Interval",
       x="Elapsed Time (minutes)", y="Requests per Second") +
  theme_minimal()

# ============================================================
# PRF-0060: throughput >= 50 req/s - combined test 2 and test 3
# ============================================================
throughput_2_5min$Test <- "Test 2 - Step"
throughput_3_5min$Test <- "Test 3 - Step (Full History)"
throughput_combined <- rbind(throughput_2_5min, throughput_3_5min)

ggplot(throughput_combined, aes(x=Interval, y=Requests.s, color=Test)) +
  geom_line(linewidth=0.85) +
  geom_hline(yintercept=50, color="black", linetype="dashed", linewidth=1) +
  scale_color_manual(values=c("Test 2 - Step"="darkorange",
                              "Test 3 - Step (Full History)"="forestgreen")) +
  labs(title="PRF-0060: Average Throughput per 5 Minute Interval",
       x="Elapsed Time (minutes)", y="Requests per Second", color="Test") +
  theme_minimal()

# ============================================================
# PRF-0070: PATCH move <= 200ms - test 3
# ============================================================
# filter for move endpoint
history_3_move <- history_3_active[history_3_active$Name == "/api/move",]

# average response time per 5 minute interval
prf_0070 <- aggregate(Total.Average.Response.Time ~ Interval,
                      data=history_3_move, FUN=mean)

# check how many intervals meet <= 200ms
passing_0070 <- sum(prf_0070$Total.Average.Response.Time <= 200)
total_0070 <- nrow(prf_0070)
cat("PRF-0070 /api/move:", passing_0070, "out of", total_0070, "intervals meet <= 200ms\n")

# plot
ggplot(prf_0070, aes(x=Interval, y=Total.Average.Response.Time)) +
  geom_line(color="steelblue", linewidth=0.85) +
  geom_hline(yintercept=200, color="red", linetype="dashed", linewidth=1) +
  labs(title="PRF-0070: Move Request Response Time",
       x="Elapsed Time (minutes)", y="Average Response Time (ms)") +
  theme_minimal()

# ============================================================
# PRF-0071: GET hint <= 500ms - test 3
# ============================================================
# filter for hint endpoint
history_3_hint <- history_3_active[history_3_active$Name == "/api/hint",]

# average response time per 5 minute interval
prf_0071 <- aggregate(Total.Average.Response.Time ~ Interval,
                      data=history_3_hint, FUN=mean)

# check how many intervals meet <= 500ms
passing_0071 <- sum(prf_0071$Total.Average.Response.Time <= 500)
total_0071 <- nrow(prf_0071)
cat("PRF-0071 /api/hint:", passing_0071, "out of", total_0071, "intervals meet <= 500ms\n")

# plot
ggplot(prf_0071, aes(x=Interval, y=Total.Average.Response.Time)) +
  geom_line(color="steelblue", linewidth=0.85) +
  geom_hline(yintercept=500, color="red", linetype="dashed", linewidth=1) +
  labs(title="PRF-0071: Hint Request Response Time",
       x="Elapsed Time (minutes)", y="Average Response Time (ms)") +
  theme_minimal()


# ============================================================
# PRF-0072: PUT undo <= 200ms - test 3
# ============================================================
# filter for undo endpoint
history_3_undo <- history_3_active[history_3_active$Name == "/api/undo",]

# average response time per 5 minute interval
prf_0072 <- aggregate(Total.Average.Response.Time ~ Interval,
                      data=history_3_undo, FUN=mean)

# check how many intervals meet <= 200ms
passing_0072 <- sum(prf_0072$Total.Average.Response.Time <= 200)
total_0072 <- nrow(prf_0072)
cat("PRF-0072 /api/undo:", passing_0072, "out of", total_0072, "intervals meet <= 200ms\n")

# plot
ggplot(prf_0072, aes(x=Interval, y=Total.Average.Response.Time)) +
  geom_line(color="steelblue", linewidth=0.85) +
  geom_hline(yintercept=200, color="red", linetype="dashed", linewidth=1) +
  labs(title="PRF-0072: Undo Request Response Time",
       x="Elapsed Time (minutes)", y="Average Response Time (ms)") +
  theme_minimal()

# ============================================================
# PRF-0073: DELETE new game <= 150ms - test 3
# ============================================================
# filter for new game endpoint
history_3_new <- history_3_active[history_3_active$Name == "/api/new",]

# average response time per 5 minute interval
prf_0073 <- aggregate(Total.Average.Response.Time ~ Interval,
                      data=history_3_new, FUN=mean)

# check how many intervals meet <= 150ms
passing_0073 <- sum(prf_0073$Total.Average.Response.Time <= 150)
total_0073 <- nrow(prf_0073)
cat("PRF-0073 /api/new:", passing_0073, "out of", total_0073, "intervals meet <= 150ms\n")

# plot
ggplot(prf_0073, aes(x=Interval, y=Total.Average.Response.Time)) +
  geom_line(color="steelblue", linewidth=0.85) +
  geom_hline(yintercept=150, color="red", linetype="dashed", linewidth=1) +
  labs(title="PRF-0073: New Game Request Response Time",
       x="Elapsed Time (minutes)", y="Average Response Time (ms)") +
  theme_minimal()

# ============================================================
# PRF-0080: GET home page <= 1000ms - test 3
# ============================================================
# filter for home endpoint
history_3_home <- history_3_active[history_3_active$Name == "/",]

# average response time per 5 minute interval
prf_0080 <- aggregate(Total.Average.Response.Time ~ Interval,
                      data=history_3_home, FUN=mean)

# check how many intervals meet <= 1000ms
passing_0080 <- sum(prf_0080$Total.Average.Response.Time <= 1000)
total_0080 <- nrow(prf_0080)
cat("PRF-0080 /:", passing_0080, "out of", total_0080, "intervals meet <= 1000ms\n")

# plot
ggplot(prf_0080, aes(x=Interval, y=Total.Average.Response.Time)) +
  geom_line(color="steelblue", linewidth=0.85) +
  geom_hline(yintercept=1000, color="red", linetype="dashed", linewidth=1) +
  labs(title="PRF-0080: Home Page Response Time",
       x="Elapsed Time (minutes)", y="Average Response Time (ms)") +
  theme_minimal()


