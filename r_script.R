
#only need to run install.packages() once ever, then comment it out
#install.packages("ggplot2")
library(ggplot2)

#setwd("Your File Path Here")

# load data
locust <- read.csv("locust_requests.csv")

stats <- read.csv("stats.csv")

head(locust)
head(stats)

# remove the aggregated row
locust_clean <- locust[locust$Name != ""]

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
stats$CPU_clean <- as.numeric(gsub("%", "", stats$CPU))

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

# Create 30 second intervals
stats$Interval <- floor(stats$Elapsed_min / 0.25) * 0.25

# Average CPU per 30 second interval
stats_30s <- aggregate(CPU_clean ~ Interval, data=stats, FUN=mean)

# Plot
ggplot(stats_30s, aes(x=Interval, y=CPU_clean)) +
  geom_line(color="steelblue", linewidth=0.85) +
  #geom_point(color="steelblue", size=2) +
  geom_hline(yintercept=80, color="red", linetype="dashed", linewidth=1) +
  labs(title="Average CPU Utilization per 30 Seconds",
       x="Elapsed Time (minutes)", y="CPU (%)") +
  theme_minimal()