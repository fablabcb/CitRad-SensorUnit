find_max_speed <- function(x){
  l <- length(x)
  m =l/2
  ii = 1:(m-1)
  max = -200
  max_i = m
  for(i in ii){
    if(x[m+i]>max){
      if(x[m+i]>x[m-i]){
        max = x[m+i]
        max_i = m+i
      }
    }
  }

  max_rev = -200
  max_i_rev = 0
  m=m
  for(i in ii){
    if(x[m-i]>max_rev){
      if(x[m-i]>x[m+i]){
        max_rev = x[m-i]
        max_i_rev = m-i
      }
    }
  }
  return(c(max=max, max_rev=max_rev, max_i=max_i, max_i_rev=max_i_rev))
}

# plot(speeds, x, type="l")
# points(speeds[max_i], max, pch=20, col="red")
# points(speeds[max_i_rev], max_rev, pch=20, col="red")

simple_runmean <- function(x, n){
  runmean = vector(mode="numeric", length=length(x))
  for(i in 2:length(x)){
    runmean[i] = ((n-1) * runmean[i-1] + x[i])/n
  }
  return(runmean)
}

read_binary_file_8bit <- function(filename, progress=F){
  size <- file.size(filename)
  con <- file(filename, open = "rb")
  # read file header:
  file_version <- readBin(con, "integer", n=1, size=2, signed = F)
  start_time <- as.POSIXct(readBin(con, "integer", n=1, size=4), tz="UTC", origin="1970-01-01")
  #start_time <- readBin(con, "integer", n=1, size=4, signed = T)
  num_fft_bins <- readBin(con, "integer", n=1, size=2, signed = F)
  iq_measurement <- readBin(con, "logical", n=1, size=1)
  sample_rate <- readBin(con, "integer", n=1, size=2, signed = F)

  # number of records:
  n <- ((size-11)/(num_fft_bins+4)) # 11 file header bytes
  n

  timestamps <- vector(mode = "integer", length=n)
  data <- matrix(nrow = n, ncol = num_fft_bins)

  # read records:
  for(i in 1:n){
    timestamps[i] <- readBin(con, "integer", n=1, size=4)
    data[i,] <- -readBin(con, "integer", n=num_fft_bins, size=1, signed = F)
    if(progress) cat(round(i/n,2), "\r")
  }
  close(con)
  return(list(data=data, timestamps=timestamps, size=size, n=n, file_version=file_version, start_time=start_time, num_fft_bins=num_fft_bins, sample_rate=sample_rate,
              iq_measurement=iq_measurement, iq_measurement=iq_measurement))
}

