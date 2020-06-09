# Cat-NB1 (NB-IoT) LwM2M Demo

When using Cat-NB1, it is not recommended to use the TCP protocol due to the network latencies inherent to NB1.  UDP is recommended for NB1 data transfer.
This demo is configured to use LwM2M and DTLS with pre-shared keys.  [LightweightM2M (LwM2M)](http://www.openmobilealliance.org/wp/Overviews/lightweightm2m_overview.html) is a protocol that runs on top of [CoAP](https://coap.technology/).

A [Leshan Server](https://www.eclipse.org/leshan/) is used to display information about the Pinnacle 100 DVK and a connected BL654 BME280 sensor.

The software version, model, manufacturer, serial number, and current time can be read.  If a BME280 sensor is connected, then
temperature, humidity, and pressure can be read.  The green LED on the DVK can be turned on and off using the light control object.

## First Time Use
If the Laird Connectivity Leshan server is used, then the Pinnacle 100 should be configured to something other than its default settings.  This will allow multiple Pinnacle 100 devices to connect to the same server.

1. Open [Laird Connectivity Leshan web page](http://uwterminalx.lairdconnect.com:8080/#/clients).
2. Go to Security page.
3. Press "Add new client configuration" button.
4. Using Pinnacle Connect mobile app, read model number from device information page.
5. With the mobile app, read IMEI from cellular page.
6. Enter <model number>_<imei> into Client endpoint field on webpage.  For example, pinnacle_100_dvk_354616090287629.
7. Generate new private shared key using the LwM2M Settings page in the mobile app.
8. Copy value into Key field on webpage.
8. Set Client ID using mobile app (don't reuse a name that is already present on the server).
9. Put the same name into the Identity field on the web page.
10. Reset the modem using the mobile app or reset button.
11. Return to the [clients page](http://uwterminalx.lairdconnect.com:8080/#/clients) and click on your board once it appears.
12. Once on your client page, make sure to set "Response Timeout" to something greater than 5s. 

## User Defined Server
In addition to the steps in the first time use section, the following must be done.
1. Write the Peer URL using mobile app.

## Development
The LwM2M demo is built with [this overlay config file](../oob_demo/overlay_lwm2m_dtls.conf).