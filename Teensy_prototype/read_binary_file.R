library(viridis)

filename <- "/Volumes/NO NAME/RECORD.BIN"

size <- file.size(filename)
n <- size/4/1025
n

timestamps <- vector(mode = "integer", length=n)
data <- matrix(nrow = n, ncol = 1024)

con <- file(filename, open = "rb")
for(i in 1:n){
  timestamps[i] <- readBin(con, "integer", n=1, size=4)
  data[i,] <- readBin(con, "double", n=1024, endian = "little", size=4)
}
close(con)

N = min(500,n) # don't plot too much data at once
image(timestamps[1:N], 1:1024, data[1:N,], col=magma(12))

sort(table(diff(timestamps)))
