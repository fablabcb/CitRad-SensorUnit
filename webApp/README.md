# CityRadar App

A web-app to display traffic data aquired by the CityRadar project.

## street shapefiles

Shapefiles come from openstreetmap. If osmdata.RData is not present data is downloaded form openstreetmap and saved into that file.

## database

The database is an sqlite database saved in the file \`radar_db.db\`. If the file is not present it is initialized with the right data fields.

## package management

Package managaement is done with with [renv](https://rstudio.github.io/renv/articles/renv.html).
