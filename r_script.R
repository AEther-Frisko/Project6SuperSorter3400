
#only need to run install.packages() once ever, then comment it out
#install.packages("ggplot2")
library(ggplot2)

#setwd("C:/Users/lexie/OneDrive/Documents/School/Classes/Software Quality IV/Project/Project6SuperSorter3400")

# load data
locust <- read.csv("locust_requests.csv")

stats_full <- read.csv("stats.csv")

head(locust)
head(stats_full)

# remove the aggregated row
locust_clean <- locust[locust$Name != "",]

# response times per endpoint
response_times <- data.frame(
  Endpoint = locust_clean$Name,
  Average_ms = locust_clean$Average.Response.Time,
  Median_ms = locust_clean$Median.Response.Time,
  Failures = locust_clean$Failure.Count,
  Requests_per_s = locust_clean$Requests.s
)

print(response_times)

# remove the aggregated row for graphing
locust_graph <- locust_clean[locust_clean$Name != "Aggregated", ]

# clean up endpoint names
locust_graph$Label <- c("Home", "Hint", "Leaderboard API", 
                        "Move", "New Game", "Start Game",
                        "Submit", "Undo", "Game Page", 
                        "Leaderboard Page")

# bar chart of average response times
ggplot(locust_graph, aes(x=Label, y=Average.Response.Time, fill=Label)) + 
  geom_bar(stat="identity") + 
  #geom_hline(yintercept=200, color="red", linetype="dashed", linewidth=1) + 
  labs(title="Average Response Time by Endpoint",
       x="Endpoint", y="Response Time (ms)") + 
  theme(axis.text.x = element_text(angle=45, hjust=1)) + 
  guides(fill="none")

# clean CPU column - remove % sign and convert to numeric
stats_full$CPU_clean <- as.numeric(gsub("%", "", stats_full$CPU))

stats_full$Time_parsed <- as.POSIXct(stats_full$Time, format="%H:%M:%S")
stats <- stats_full[stats_full$Time_parsed >= as.POSIXct("19:13:00", format="%H:%M:%S"), ]


# convert time to elapsed minutes
stats$Time_parsed <- as.POSIXct(stats$Time, format="%H:%M:%S")
stats$Elapsed_min <- as.numeric(difftime(stats$Time_parsed, stats$Time_parsed[1], units="mins"))

# plot CPU over time
ggplot(stats, aes(x=Elapsed_min, y=CPU_clean)) +
  geom_line(color="steelblue") +
  geom_hline(yintercept=80, color="red", linetype="dashed", linewidth=1) +
  labs(title="CPU Utilization During Load Test",
       x="Elapsed Time (minutes)", y="CPU (%)") +
  theme_minimal()

# create 15 second intervals
stats$Interval <- floor(stats$Elapsed_min / 0.25) * 0.25

# average CPU per 15 second interval
stats_15s <- aggregate(CPU_clean ~ Interval, data=stats, FUN=mean)

# plot
ggplot(stats_15s, aes(x=Interval, y=CPU_clean)) +
  geom_line(color="steelblue", linewidth=0.85) +
  #geom_point(color="steelblue", size=2) +
  geom_hline(yintercept=80, color="red", linetype="dashed", linewidth=1) +
  labs(title="Average CPU Utilization per 15 Seconds",
       x="Elapsed Time (minutes)", y="CPU (%)") +
  theme_minimal()

# extract just the used memory value and convert to numeric
stats$Memory_clean <- as.numeric(gsub("MiB.*", "", stats$Memory))

# create 15 second intervals
stats$Interval <- floor(stats$Elapsed_min / 0.25) * 0.25

# average memory per 15 second interval
stats_mem <- aggregate(Memory_clean ~ Interval, data=stats, FUN=mean)

# plot
ggplot(stats_mem, aes(x=Interval, y=Memory_clean)) +
  geom_line(color="steelblue", linewidth=0.85) +
  labs(title="Memory Usage During Load Test",
       x="Elapsed Time (minutes)", y="Memory (MiB)") +
  theme_minimal()

# extract user count over time from locust report data
# the data shows users ramping from 1 to 100 starting at 23:13:55

# create the ramp up data from the report
locust_time <- as.POSIXct(c(
  "2026-04-01 23:13:55", "2026-04-01 23:33:51"
), format="%Y-%m-%d %H:%M:%S")

# calculate elapsed minutes for each data point in the html
# start time was 23:13:55, hit 100 users at 23:33:51 (about 20 minutes)

# build user count over time from the report data
report_start <- as.POSIXct("2026-04-01 23:13:55", format="%Y-%m-%d %H:%M:%S")

# key timestamps from the data
user_times <- c(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 80)
user_counts <- c(1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 100)

# use the actual data - ramp from 0 to 100 over ~20 minutes then hold
elapsed <- seq(0, 80, by=0.083)
users <- pmin(floor(elapsed / 20 * 100), 100)
users[1] <- 0
ramp_data <- data.frame(Elapsed=elapsed, Users=users)

# plot
ggplot(ramp_data, aes(x=Elapsed, y=Users)) +
  geom_line(color="steelblue", linewidth=0.85) +
  geom_hline(yintercept=100, color="red", linetype="dashed", linewidth=1) +
  labs(title="Simulated Users Over Time",
       x="Elapsed Time (minutes)", y="Number of Users") +
  theme_minimal()