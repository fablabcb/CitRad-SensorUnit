library(shiny)

function(input, output, session) {

  output$map = renderLeaflet({


    # tm_shape(street_shapes) +
    #   tm_fill(col="gray") +
    #   tm_shape(buildings) +
    #   tm_fill(col="gray90") +
    #   tm_shape(streams) +
    #   tm_fill(col="lightblue") +
    #   tm_shape(water) +
    #   tm_fill(col="lightblue") +


    # tm_basemap("OpenStreetMap", alpha = .3) +
    #   tm_shape(street_multi_lane) +
    #   tm_lines(col="black") +
    #   tm_shape(street_1_lane) +
    #   tm_lines(col="blue") +
    #   tm_shape(street_NA_lane) +
    #   tm_lines(col="orange") +
    #   tm_shape(ATKIS_AX_Fahrbahnachse) +
    #   tm_lines(col="darkgreen") +
    #   # tm_shape(right_lane) +
    #   # tm_lines(col="red") +
    #   # tm_shape(left_lane) +
    #   # tm_lines(col="green") +
    #   # tm_shape(crossings) +
    #   # tm_dots(col="gray40", size=.01) +
    #   tm_shape(ATKIS_rail) +
    #   tm_lines(col="black") +
    #   tm_shape(ATKIS_Bahnstrecke) +
    #   tm_lines(col="black")

    tmap_leaflet(
      tm_basemap("OpenStreetMap", alpha = .2) +
        tm_shape(data$osm_lines %>% filter(highway!="path") %>% select("osm_id", "name", "name:hsb", "highway", "lanes", "maxspeed", "oneway", "vehicle")) +
        tm_lines(col="maxspeed", lwd = 2, id="osm_id", popup.vars = FALSE) #c("name", "name:hsb", "highway", "lanes", "maxspeed", "oneway", "vehicle")
    ) #%>%
      #addArrowhead(data = data$osm_lines %>% filter(oneway=="yes" & highway != "path"), options = arrowheadOptions(size = "3m"))

  })

  observe({
    print(input$map_shape_click$id)
  })

  selected_id <- reactive({
    id = req(input$map_shape_click$id)
    id = id %>% str_remove("X")
    return(id)
  })

  selected_street <- reactive({
    data$osm_lines %>%
      filter(osm_id==selected_id())
  })

  output$selected_name <- renderUI({
    street <- req(selected_street())
    div(p(street$name),
        p(street$`name:hsb`),
        p("maximal", street$maxspeed, "km/h"),
        p(street$lanes, " Fahrbahnen"),
        p(ifelse(!is.na(street$oneway) && street$oneway=="yes", "Einbahnstraße", "Gegenverkehr"))
    )
  })

  observe({
    if(input$reverse){
      street = selected_street() %>% st_reverse()
    }else{
      street = selected_street()
    }

    leafletProxy("map", session) %>%
      #addPolylines(selected_street(), color = "blue") %>%
      removeArrowhead("selected_street") %>%
      addArrowhead(data = street, options = arrowheadOptions(size = "10px", frequency = "30px"), layerId = "selected_street")
  })

  #========= database connection ============

  update_sensors <- reactiveVal(0)
  update_locations <- reactiveVal(0)

  # DBI::dbExecute(user_db, str_glue("DELETE FROM logger_settings WHERE userID = '{userID()}'"))
  observeEvent(input$create_sensor, {
    # TODO: check if length < 50
    DBI::dbExecute(radar_db, str_glue("INSERT INTO sensors(sensor_label) VALUES('{input$sensor_label}')" ))
    # TODO: react if query goes wrong
    # TODO: message if sensor is successfully created
    update_sensors(update_sensors()+1)
  })

  sensors <- reactive({
    update_sensors()
    DBI::dbGetQuery(radar_db, str_glue("SELECT sensorID, sensor_label FROM sensors;"))
  })

  locations <- reactive({
    update_locations()
    locations <- DBI::dbGetQuery(radar_db, str_glue("SELECT * FROM locations;")) %>%
      mutate(start = as.POSIXct(start, origin = "1970-01-01")) %>%
      mutate(end = as.POSIXct(end, origin = "1970-01-01")) %>%
      as_tibble

    data$osm_lines %>%
      select(osm_id, name) %>%
      st_drop_geometry() %>%
      mutate(osm_id = as.integer(osm_id)) %>%
      right_join(locations, by="osm_id")
  })

  observe({
    # TODO: update if sensor is added
    if(nrow(sensors())>0){
      choices = sensors()$sensorID
      names(choices) = paste0(sensors()$sensorID, " (", sensors()$sensor_label, ")")
      updateSelectInput(session, "sensor_selection", choices = choices, selected = NULL)
    }
  })

  observeEvent(input$place_sensor, {
    # TODO: check if all inputs are available
    coordinates <- st_endpoint(selected_street()) %>% st_coordinates() %>% as_tibble

    DBI::dbExecute(radar_db, str_glue("UPDATE locations SET end='{as.integer(Sys.time())}' WHERE sensorID = '{input$sensor_selection}' AND end IS NULL" ))


    DBI::dbExecute(radar_db, str_glue("INSERT INTO locations(osm_id, sensorID, reverse, start, longitude, latitude) VALUES('{selected_id()}','{input$sensor_selection}', '{input$reverse}', '{round(as.integer(Sys.time()))}', '{coordinates$X}', '{coordinates$Y}');" ))
    # TODO: react if query goes wrong

    # TODO: update stop time at last position
    update_locations(update_locations()+1)
  })

  output$sensor_table <- renderRHandsontable({
    rhandsontable(sensors())
  })

  output$locations_table <- renderRHandsontable({
    rhandsontable(locations())
  })
}
#
# CREATE TABLE locations (
#   osm_id INTEGER NOT NULL,
#   sensorID INTEGER NOT NULL,
#   `direction` INTEGER NOT NULL,
#   `start` INTEGER NOT NULL,
#   `end` INTEGER,
#   `longitude` DOUBLE,
#   `latitude` DOUBLE,
#   `height` DOUBLE
#   `installation_notes` TEXT,
#   PRIMARY KEY (sensorID, `timestamp`))
