# CitRad Radar Sensor data processing method

We use R to prototype and test the data processing later to be implemented on the sensor unit itself. 


## Setup R environment:

Install R from [r-project.org](https://www.r-project.org/). Also we suggest using the [RStudio IDE](https://posit.co/downloads/).

Open the project by opening the `.Rproj` file in RStudio.

We use [renv](https://rstudio.github.io/renv/index.html) to keep track of package versions. Execute `renv::restore()` to install all packages recorded in `renv.lock`. Maybe you have to install renv first with `install.packages("renv")`.

