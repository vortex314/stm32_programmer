# ESP8266 bootloader driver for STM32
Program implements :
- UDP listener on port 1883
- Listens for JSON formed UDP packets
 {"id":123,"request":"status"}
- Sends UART inputs in raw mode back to last sender
- Sends LOGS in raw mode back to last sender
- Executes following BOOTLOADER STM32 commands 

| Command        | JSON           | DESC  |
| ------------- |:-------------:| -----:|
| reset      | {"request":"reset","id":12} | pulse reset pin of stm32 and send at 115200 the sync char |
| col 2 is      | centered      |   $12 |
| zebra stripes | are neat      |    $1 |




