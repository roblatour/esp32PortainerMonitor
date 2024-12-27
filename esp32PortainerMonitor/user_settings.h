#define SETTINGS_PORTAINER_SERVER              "192.168.1.15"	     
#define SETTINGS_PORTAINER_SERVER_PORT         9000

#define SETTINGS_PORTAINER_ENDPOINT_ID         ""              // On your first run of this program leave this value empty ("") and the program will automatically determine your endpoint id
                                                               //
                                                               // After the first run:
                                                               //  you can either continue to leave this value set to "" 
                                                               //  or 
                                                               //  to save the program looking the endpoint id each time it starts:
                                                               //    look in the Serial Monitor output from the first run for the line that reads:
                                                               //        Endpoint ID found, value is: n  
                                                               //        (where n is a number, for example: 2)
                                                               //    you can then replace the "" above with the number displayed in the Serial Montior output,
                                                               //    for example "2"

#define SETTINGS_TFT_WIDTH                      240            // TFT Width in pixels
#define SETTINGS_TFT_HEIGHT                     320            // TFT Height in pixels

#define SETTINGS_MAX_BUFFER_ROWS                200            // max rows in the virutal window
#define SETTINGS_MAX_BUFFER_COLUMNS              64            // max columns in the virutal window

#define SETTINGS_REFRESH_RATE_IN_SECONDS         60            // refresh rate in seconds