# BUGS

- io.TTY.set_raw() stopped working correctly. Run io test to confirm.
- segmentation fault on multiple direct iterations of any hash method.
    <br><br>REPRODUCTION:
  
    ```blade
    import hash
    for i in 1..100000 {
        echo hash.sha1('hello')
    }
   ```
