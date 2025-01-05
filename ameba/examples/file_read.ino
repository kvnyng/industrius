/*
 This sketch shows how to open/close file and perform read/write to it.

 Example guide:
 https://www.amebaiot.com/en/amebapro2-arduino-filesystem-simple-application/
 */

#include "AmebaFatFS.h"

char filename[] = "sample.jpeg";

AmebaFatFS fs;

void setup()
{
    char buf[128];
    char path[128];

    fs.begin();

    sprintf(path, "%s%s", fs.getRootPath(), filename);
    File file = fs.open(path);

    memset(buf, 0, sizeof(buf));
    file.read(buf, sizeof(buf));

    file.close();
    printf("==== content ====\r\n");
    printf("%s", path);
    printf("%s", buf);
    printf("====   end   ====\r\n");

    fs.end();
}

void loop()
{
    delay(1000);
}
