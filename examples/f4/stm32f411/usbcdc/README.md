# Usb CDC

Echoes back what you write via usb. Test with 
```
cat /dev/ttyACM0
```
And in another terminal tab:
```
echo "test123" > /dev/ttyACM0
```

If it works you will see the text in the first terminal tab. 
