## Task description

2 line progress bar in matrix: 1st row shows overall progress, 2nd row shows progress of "current file". Current file LED-s turn on 1 by 1, when they are all turned on, the main progress bar progresses by 2 LEDs.

Notes:

It seems that based on the description it is not possible to have concurrent file uploads, so there is no need for keeping track of what file and how much it uploaded in parallel.

## Possible solutions

ESP1 - file progress reporter
ESP2 - LED matrix display and file progress status "handler"

1. (for testing and demo) ESP listens for mqtt topic to start the upload process. MQTT topic can include some config for the file upload process.
2. ESP1 sends signal for initializing file upload, mqtt message includes number of files and/or total size of all files

### Topics

* /<ESP#>/file/upload/start/init

ESP1 listens to this topic, when received - triggers the file upload process

* /<ESP#>/file/upload/start

``` json
{
    "total_size_mb": 1024,
    "num_of_files": 10
}
```

/<ESP#>/file/upload/status
OR
/<ESP#>/file/upload/<file_name>/status
OR
generate a unique file upload session ID (seems to be best solution)
/<ESP#>/file/upload/<session_id>/status

``` json
{
    "file_name": "wasd.png",
    "uploaded_mb": 16,
    "total_size": 128,
}
```

* last message for each file should have `uploaded_mg` = `total_size`
* Does the receiver need to confirm any of these messages? With QoL >= 2 it should not be a problem, also this is very beginner course, so this can be done later or mentioned in the report.