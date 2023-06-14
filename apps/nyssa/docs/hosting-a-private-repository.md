# Hosting a private repository

Nyssa is a self-hostable repository.

This means that you can install and configure your own private or custom instance of [nyssa.bladelang.com](https://nyssa.bladelang.com) that people can publish to and install packages from. In fact, [nyssa.bladelang.com](https://nyssa.bladelang.com) is essentially just another private repository that is accessible to everyone in the Blade community and hosted on the public web.

### Starting the respository server

Once you have Nyssa installed on a device, you can start the repository server by running the command `nyssa serve`. This starts a local repository server that listens to the port `3000`. You should see a log similar to the following:

```
$ nyssa serve
Nyssa repository server started.
Repository URL: http://127.0.0.1:3000
```

You can visit `http://127.0.0.1:3000` in your browser to see your own clone of this website. 

Replace IP address with the address/domain of the server if you are trying to access it outside of the server. In fact, if you'll be accessing it ourside the local machine, it is advisable to change the configured host from `127.0.0.1` to `0.0.0.0`.

### Customizing your installation

You fully customize your own repository setup by modifying the `app/setup.b` file. For exmple, you can change the host and port number by modifying the value of `REPOSITORY_HOST` and `REPOSITORY_PORT` respectively in the file `app/setup.b`.

### Production considerations

- While the built-in server can be used as is on production, it is not advisable to do so unless you are not exposing it on the internet (such as internal use on in an organization). Especially if you are trying to run on `https` as the server only supports plain `http`.
- Consider running behind a reverse-proxy such as `apache` or `lightspeed` or `nginx` or run it behind a `VPN`.

