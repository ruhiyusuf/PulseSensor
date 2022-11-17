# RaspberryPi w/ Pulse Sensor

[Working Doc](https://docs.google.com/document/d/1kU0XOYXyPuss5QQA799x6_3WqHYbQQyjto_E0LJ5l_I/edit#heading=h.6qdqgkb7b2o2)

Final Implementation: 

* Deploy `arduino_hr_ibi/arduino_hr_ibi.ino` to Arduino to read values from Pulse Sensor.

* Run `udp_test/server.py` to read heartrate + ibi values from Arduino through Serial Port. (Add to crontab, cmd is in `crontab.txt`)

* Run `udp_test/client.py` to listen to heartrate + ibi values from terminal

To receive values in Unity: Create an object and `unity/UDPListner.cs` as a component. 

[Graybox Implementation for HeartRate + UDP Listening in VR](https://github.com/NitishGourishetty/UDPGrayboxHRVPI)
