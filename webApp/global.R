library(sf)
library(geos)
library(readr)
library(dplyr)
library(stringr)
library(units)
library(tmap)
library(lwgeom)
library(leaflet)
library(leaflet.extras2)
library(osmdata)
library(DBI)
library(RSQLite)
library(rhandsontable)



WGS_crs <- st_crs(4326)    # coordinate system of the bbox function
UTM_33_crs <- st_crs(25833)   # coordinate system of the dem query

Cottbus_1km_UTM_33 <-
  st_point(c(453888, 5734646)) %>%      # center point of Cottbus in UTM 33N Coordinates
  st_sfc() %>%                          # encapsulate in sfc (simple feature geometry) object
  st_set_crs(UTM_33_crs) %>%             # convert to target crs
  st_buffer(dist=set_units(5000, m)) %>%  # add a buffer of 1km around it
  st_bbox() %>%                         # calculate bounding box of buffered zone
  round()       # round coordinates (important, since the data query doesn't work with floats)

Cottbus_1km_UTM_33_txt <- Cottbus_1km_UTM_33 %>% paste(collapse=",")

# if(file.exists("shapefiles.RData")){
#   message("loading shapefiles from file")
#   load("shapefiles.RData")
# }else{
#   message("downloading shapefiles")
#   ATKIS_streets_axis <- read_sf(str_glue("https://isk.geobasis-bb.de/ows/atkisbdlm_sf_wfs?Service=WFS&REQUEST=GetFeature&VERSION=2.0.0&TYPENAMES=adv:AX_Strassenachse&COUNT=100000&BBOX={Cottbus_1km_UTM_33_txt},urn:ogc:def:crs:EPSG::25833"))
#   ATKIS_AX_Fahrbahnachse <- read_sf(str_glue("https://isk.geobasis-bb.de/ows/atkisbdlm_sf_wfs?Service=WFS&REQUEST=GetFeature&VERSION=2.0.0&TYPENAMES=adv:AX_Fahrbahnachse&COUNT=100000&BBOX={Cottbus_1km_UTM_33_txt},urn:ogc:def:crs:EPSG::25833"))
#   street_shapes <- read_sf(str_glue("https://isk.geobasis-bb.de/ows/alkis_sf_wfs?Service=WFS&REQUEST=GetFeature&VERSION=2.0.0&TYPENAMES=adv:AX_Strassenverkehr&COUNT=10000&BBOX={Cottbus_1km_UTM_33_txt},urn:ogc:def:crs:EPSG::25833"))
#   buildings <- read_sf(str_glue("https://isk.geobasis-bb.de/ows/alkis_sf_wfs?Service=WFS&REQUEST=GetFeature&VERSION=2.0.0&TYPENAMES=adv:AX_Gebaeude&COUNT=10000&BBOX={Cottbus_1km_UTM_33_txt},urn:ogc:def:crs:EPSG::25833"))
#   streams <- read_sf(str_glue("https://isk.geobasis-bb.de/ows/alkis_sf_wfs?Service=WFS&REQUEST=GetFeature&VERSION=2.0.0&TYPENAMES=adv:AX_Fliessgewaesser&COUNT=10000&BBOX={Cottbus_1km_UTM_33_txt},urn:ogc:def:crs:EPSG::25833"))
#   water <- read_sf(str_glue("https://isk.geobasis-bb.de/ows/alkis_sf_wfs?Service=WFS&REQUEST=GetFeature&VERSION=2.0.0&TYPENAMES=adv:AX_StehendesGewaesser&COUNT=10000&BBOX={Cottbus_1km_UTM_33_txt},urn:ogc:def:crs:EPSG::25833"))
#   ATKIS_rail <- read_sf(str_glue("https://isk.geobasis-bb.de/ows/atkisbdlm_sf_wfs?Service=WFS&REQUEST=GetFeature&VERSION=2.0.0&TYPENAMES=adv:AX_Gleis&COUNT=100000&BBOX={Cottbus_1km_UTM_33_txt},urn:ogc:def:crs:EPSG::25833"))
#   ATKIS_Bahnstrecke <- read_sf(str_glue("https://isk.geobasis-bb.de/ows/atkisbdlm_sf_wfs?Service=WFS&REQUEST=GetFeature&VERSION=2.0.0&TYPENAMES=adv:AX_Bahnstrecke&COUNT=100000&BBOX={Cottbus_1km_UTM_33_txt},urn:ogc:def:crs:EPSG::25833"))
#
#   save(ATKIS_streets_axis, ATKIS_AX_Fahrbahnachse, street_shapes, buildings, streams, ATKIS_rail, ATKIS_Bahnstrecke, file="shapefiles.RData")
# }

if(file.exists("osmdata.RData")){
  message("loading osm shapefiles from file")
  load("osmdata.RData")
}else{
  message("loading openstreetmap data")

  data <- opq("Cottbus") %>%
    add_osm_feature(key="highway", value=c("unclassified", "primary", "secondary", "tertiary", "residential", "living_street", "!service", "path")) %>%
    osmdata_sf()
  # secondary = Landstraße

  save(data, file = "osmdata.RData")
}

# street_multi_lane <- ATKIS_streets_axis %>% filter(anzahlDerFahrstreifen>=2)
# street_1_lane <- ATKIS_streets_axis %>% filter(anzahlDerFahrstreifen==1)
# street_NA_lane <- ATKIS_streets_axis %>% filter(is.na(anzahlDerFahrstreifen))
#
# right_lane <- geos_offset_curve(ATKIS_streets_axis, 3) %>% st_as_sf(offset)
# left_lane <- geos_offset_curve(ATKIS_streets_axis, -3) %>% st_as_sf(offset)
# crossings <- st_startpoint(ATKIS_streets_axis)
# #crossings2 <- st_endpoint(ATKIS_streets_axis)

#====== generate databases =============

radar_db <- DBI::dbConnect(RSQLite::SQLite(), dbname = "radar_db.db")


if(!dbExistsTable(radar_db, "sensors")){
  DBI::dbExecute(radar_db, "CREATE TABLE sensors (
                 sensorID INTEGER PRIMARY KEY NOT NULL UNIQUE,
                 `sensor_label` varchar(50))")
}

if(!dbExistsTable(radar_db, "locations")){
  DBI::dbExecute(radar_db, "CREATE TABLE locations (
                 sensorID INTEGER NOT NULL,
                 osm_id INTEGER NOT NULL,
                 `reverse` INTEGER NOT NULL,
                 `start` INTEGER NOT NULL,
                 `end` INTEGER,
                 `longitude` DOUBLE,
                 `latitude` DOUBLE,
                 `height` DOUBLE
                 `installation_notes` TEXT,
                 PRIMARY KEY (osm_id, sensorID, start, end))")
}
# direction is as is = 0 (same as osm shapefile) or reverse = 1


if(!dbExistsTable(radar_db, "measurements")){
  DBI::dbExecute(radar_db, "CREATE TABLE measurements (
                 sensorID INTEGER NOT NULL,
                 `timestamp` INTEGER NOT NULL,
                 `speed` INTEGER NOT NULL,
                 `reverse` INTEGER NOT NULL,
                 `certainty` DOUBLE,
                 `perceived_target_size` DOUBLE,
                 `vehicle_type` INTEGER,
                 PRIMARY KEY (sensorID, `timestamp`))")
}

if(!dbExistsTable(radar_db, "vehicle_types")){
  DBI::dbExecute(radar_db, "CREATE TABLE vehicle_types (
                 vehicle_typeID INTEGER NOT NULL,
                 `name` varchar(50) NOT NULL,
                 `description` TEXT NOT NULL,
                 `mean_perceived_target_size` DOUBLE,
                 `oscillates` INTEGER,
                 PRIMARY KEY (vehicle_typeID))")
}

onStop(function() {
  dbDisconnect(radar_db)
})
