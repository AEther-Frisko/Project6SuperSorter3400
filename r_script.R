# ============================================================
# packages
# ============================================================
# install.packages("ggplot2")
library(ggplot2)

setwd("C:/Users/lexie/OneDrive/Documents/School/Classes/Software Quality IV/Project/Project6SuperSorter3400")

# ============================================================
# load data
# ============================================================
# test 1 - gradual ramp up
locust_1 <- read.csv("1_locust_requests.csv")
stats_1_full <- read.csv("1_stats.csv")

# test 2 - step ramp up
locust_2 <- read.csv("2_locust_requests.csv")
stats_2_full <- read.csv("2_stats.csv")
history_2 <- read.csv("2_locust_stats_history.csv")

# ============================================================
# data prep - test 1
# ============================================================
locust_1_clean <- locust_1[locust_1$Name != "",]

stats_1_full$CPU_clean <- as.numeric(gsub("%", "", stats_1_full$CPU))
stats_1_full$Memory_clean <- as.numeric(gsub("MiB.*", "", stats_1_full$Memory))
stats_1_full$Time_parsed <- as.POSIXct(stats_1_full$Time, format="%H:%M:%S")

stats_1 <- stats_1_full[stats_1_full$Time_parsed >= as.POSIXct("19:13:00", format="%H:%M:%S"),]
stats_1$Time_parsed <- as.POSIXct(stats_1$Time, format="%H:%M:%S")
stats_1$Elapsed_min <- as.numeric(difftime(stats_1$Time_parsed, stats_1$Time_parsed[1], units="mins"))
stats_1$Interval <- floor(stats_1$Elapsed_min / 0.25) * 0.25

# ============================================================
# data prep - test 2
# ============================================================
locust_2_clean <- locust_2[locust_2$Name != "",]

stats_2_full$CPU_clean <- as.numeric(gsub("%", "", stats_2_full$CPU))
stats_2_full$Memory_clean <- as.numeric(gsub("MiB.*", "", stats_2_full$Memory))
stats_2_full$Time_parsed <- as.POSIXct(stats_2_full$Time, format="%H:%M:%S")

stats_2 <- stats_2_full[stats_2_full$Time_parsed >= as.POSIXct("01:15:00", format="%H:%M:%S"),]
stats_2$Time_parsed <- as.POSIXct(stats_2$Time, format="%H:%M:%S")
stats_2$Elapsed_min <- as.numeric(difftime(stats_2$Time_parsed, stats_2$Time_parsed[1], units="mins"))
stats_2$Interval <- floor(stats_2$Elapsed_min / 0.25) * 0.25

history_2$Time_parsed <- as.POSIXct(history_2$Timestamp, origin="1970-01-01", tz="UTC")
history_2$Elapsed_min <- as.numeric(difftime(history_2$Time_parsed, history_2$Time_parsed[1], units="mins"))
history_2_active <- history_2[history_2$User.Count > 0,]
history_2_active$Elapsed_min <- as.numeric(difftime(history_2_active$Time_parsed, history_2_active$Time_parsed[1], units="mins"))
history_2_active$Interval <- floor(history_2_active$Elapsed_min / 5) * 5

# ============================================================
# PRF-0040: 100 concurrent users - combined PASS
# ============================================================
ramp_combined <- data.frame(
  Elapsed = c(elapsed_1, elapsed_2),
  Users = c(users_1, users_2),
  Test = c(rep("Test 1 - Gradual", length(elapsed_1)),
           rep("Test 2 - Step", length(elapsed_2)))
)

ggplot(ramp_combined, aes(x=Elapsed, y=Users, color=Test)) +
  geom_line(linewidth=0.85) +
  geom_hline(yintercept=100, color="black", linetype="dashed", linewidth=1) +
  scale_color_manual(values=c("Test 1 - Gradual"="steelblue", "Test 2 - Step"="darkorange")) +
  labs(title="Simulated Users Over Time",
       x="Elapsed Time (minutes)", y="Number of Users", color="Test") +
  theme_minimal()

# ============================================================
# PRF-0050: CPU utilization <= 80% - combined PASS
# ============================================================
stats_1_cpu$Test <- "Test 1 - Gradual"
stats_2_cpu$Test <- "Test 2 - Step"
cpu_combined <- rbind(stats_1_cpu, stats_2_cpu)

ggplot(cpu_combined, aes(x=Interval, y=CPU_clean, color=Test)) +
  geom_line(linewidth=0.85) +
  geom_hline(yintercept=80, color="black", linetype="dashed", linewidth=1) +
  scale_color_manual(values=c("Test 1 - Gradual"="steelblue", "Test 2 - Step"="darkorange")) +
  labs(title="CPU Utilization During Load Test",
       x="Elapsed Time (minutes)", y="CPU (%)", color="Test") +
  theme_minimal()

# ============================================================
# memory usage - combined PASS
# ============================================================
stats_1_mem$Test <- "Test 1 - Gradual"
stats_2_mem$Test <- "Test 2 - Step"
mem_combined <- rbind(stats_1_mem, stats_2_mem)

ggplot(mem_combined, aes(x=Interval, y=Memory_clean, color=Test)) +
  geom_line(linewidth=0.85) +
  ylim(0, 150) +
  scale_color_manual(values=c("Test 1 - Gradual"="steelblue", "Test 2 - Step"="darkorange")) +
  labs(title="Memory Usage During Load Test",
       x="Elapsed Time (minutes)", y="Memory (MiB)", color="Test") +
  theme_minimal()

# ============================================================
# PRF-0060: throughput >= 50 req/s - test 2 FAIL
# ============================================================
throughput_5min <- aggregate(Requests.s ~ Interval, data=history_2_active, FUN=mean)

passing_tp <- sum(throughput_5min$Requests.s >= 50)
total_tp <- nrow(throughput_5min)
cat("PRF-0060 Test 2:", passing_tp, "out of", total_tp, "intervals meet >= 50 req/s\n")

ggplot(throughput_5min, aes(x=Interval, y=Requests.s)) +
  geom_line(color="steelblue", linewidth=1.5) +
  geom_hline(yintercept=50, color="red", linetype="dashed", linewidth=1) +
  labs(title="Test 2: Average Throughput per 5 Minute Interval",
       x="Elapsed Time (minutes)", y="Requests per Second") +
  theme_minimal()