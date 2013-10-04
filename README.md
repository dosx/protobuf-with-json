protobuf-with-json
===============

Protobuf compiler plugin with json and service support. For now only java is supported, other languages may or may not be coming soon.

The idea is to define your server's json api in protobuf language, and then use this compiler to generate client bindings (for example, Android and iOS clients) that are strongly typed, easy to use, and provide safe serialization and parsing to and from JSON. For more information on protobuf, see https://code.google.com/p/protobuf/.

An example, define your server's api like so:
```
message GetBalanceRequest {
  optional bool include_all_accounts = 1;
}

message GetBalanceResponse {
  message AccountBalance {
    optional string name = 1;
    optional double balance = 2;
  }

  // Total balance of the accounts.
  optional double total_balance = 1;

  // Map of account name -> balance.
  repeated AccountBalance accounts = 2 [(dx_map_key)="name", (dx_map_val)="balance"];
}

service Bank {
  rpc GetBalanceCall (GetBalanceRequest) returns (GetBalanceResponse) {
    option (dx_method_options).path = "/user/:userId/get_balance";
    option (dx_method_options).http_method = "POST";
  }
}
```

The compiler will generate an abstract class like this:
```
  public static abstract class Bank {
    public static interface Callback<T> {
        void done(int code, String error, T response);
    }

    protected abstract <T> void doCall(
        final String path,
        final String httpMethod,
        final org.json.JSONObject params,
        final Class<T> responseType,
        final Callback<T> callback);

    public void GetBalanceCall(String userId, com.example.MyExample.GetBalanceRequest req, final Callback<com.example.MyExample.GetBalanceResponse> callback) {
      String path = "/user/" + userId + "/get_balance";
      org.json.JSONObject params = null;
      try {
        params = req.toJSON();
      } catch (org.json.JSONException e) {
        callback.done(-1, "JSON error: " + e, null);
        return;
      }
      this.doCall(path, "POST", params, com.example.MyExample.GetBalanceResponse.class, callback);
    }
  }

```

Then, you would subclass Bank and implement doCall to actually send the request to your server and receive and transform the response.


To build the compiler:
```
$ make
```

To build the example proto:
```
$ make example
```
