library(shiny)

fluidPage(

    # Application title
    titlePanel("CityRadar"),

    sidebarLayout(
        sidebarPanel(
          h4("Neuer Sensor"),
          wellPanel(
            textInput("sensor_label", "Sensor Label (optional)"),
            actionButton("create_sensor", "Sensor anlegen")
          ),
          h4("Sensor platzieren"),
          wellPanel(
            htmlOutput("selected_name"),
            checkboxInput("reverse", "Messrichtung umkehren"),
            selectInput("sensor_selection", "Sensor", selected = NA, choices=NULL),
            actionButton("place_sensor", "Sensor platzieren")
          )
        ),

        mainPanel(
          tmapOutput("map", height = "500px"),
          h4("Sensor Übersicht"),
          wellPanel(
            h3("sensors"),
            rHandsontableOutput("sensor_table"),
            h3("locations"),
            rHandsontableOutput("locations_table")
          )
        )
    )
)
