library(viridis)

filename <- "/Volumes/NO NAME/RECORD.BIN"

size <- file.size(filename)
con <- file(filename, open = "rb")
# read file header:
file_version <- readBin(con, "integer", n=1, size=2, signed = F)
start_time <- as.POSIXct(readBin(con, "integer", n=1, size=4, signed = F), tz="UTC", origin="1970-01-01")
num_fft_bins <- readBin(con, "integer", n=1, size=2, signed = F)
iq_measurement <- readBin(con, "logical", n=1, size=1)
sample_rate <- readBin(con, "integer", n=1, size=2, signed = F)

# number of records:
n <- (size-11)/4/(num_fft_bins+1) # 11 file header bytes

timestamps <- vector(mode = "integer", length=n)
data <- matrix(nrow = n, ncol = num_fft_bins)

# read records:
for(i in 1:n){
  timestamps[i] <- readBin(con, "integer", n=1, size=4)
  data[i,] <- readBin(con, "double", n=num_fft_bins, endian = "little", size=4)
}
close(con)

# plot data
N = min(500,n) # don't plot too much data at once
image(timestamps[1:N], 1:1024, data[1:N,], col=magma(12))

# show statistic of timesteps between records:
sort(table(diff(timestamps)))
