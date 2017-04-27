var socket;
var isAccEnabled = true;

$(document).ready(function() {
  var lastMove = 0;
  
  //Listener for buttons

  $("a.forward").click(function() {
    move(800, 800);
  });
  $("a.back").click(function() {
    move(-800, -800);
  });
  $("a.left").click(function() {
    move(800, 0);
  });
  $("a.right").click(function() {
    move(0, 800);
  });
  $("a.accelerator").click(function() {
    isAccEnabled = !isAccEnabled;
    if (isAccEnabled)
      document.getElementById("dmEvent").innerHTML = "Accelerometer enabled";
    else
      document.getElementById("dmEvent").innerHTML = "Accelerometer disabled";
  })


  $("a.start").click(function() {
    var obj = {
      type: "engine",
      cmd: "start"
    };
    socket.send(JSON.stringify(obj));
  });
  $("a.stop").click(function() {
    move(0, 0);
    var obj = {
      type: "engine",
      cmd: "stop"
    };
    socket.send(JSON.stringify(obj));
  });

  //Check for if it's a mobile device or not
  if (window.DeviceMotionEvent) {
    window.addEventListener('devicemotion', deviceMotionHandler, false);
  } else {
    console.log("Accelerometer not supported.")
    document.getElementById("dmEvent").innerHTML = "Accelerometer not supported."
  }

  //For Keyboard control - WASD
  document.onkeydown = function detectKey(event) {
    var e = event.keyCode;
    if (e == 87) { //W
      move(800, 800);
    }
    if (e == 83) { //S
      move(-800, -800);
    }
    if (e == 65) { //A
      move(-800, 800);
    }
    if (e == 68) { //D
      move(800, -800);
    }
  }

  //Actual functions for movement
  function move(left, right) {
    var now = Date.now();
    if (lastMove + 200 < now) {
      lastMove = now;
      var obj = {
        type: "engineValue",
        left: left,
        right: right
      };
      socket.send(JSON.stringify(obj));
    }
  }

  //Accelerometer function for device control
  function deviceMotionHandler(eventData) {
    if (isAccEnabled) {
      acceleration = eventData.accelerationIncludingGravity;
      var left = 0;
      var right = 0;
      if (Math.abs(acceleration.y) > 1) { // back-/forward
        var speed = acceleration.y * 150;
        left = Math.min(1023, speed + acceleration.x * 100);
        right = Math.min(1023, speed - acceleration.x * 100);
      } else if (Math.abs(acceleration.x) > 1) {
        var speed = Math.min(1023, Math.abs(acceleration.x) * 123);
        if (acceleration.x > 0) {
          left = speed;
          right = -speed;
        } else {
          left = -speed;
          right = speed;
        }
      }
      if (Math.abs(left) > 100 || Math.abs(right) > 100) {
        if (left < -1023) {
          left = -1023;
        }
        if (right < -1023) {
          right = -1023;
        }
        move(left, right);
      }
      var direction = "stop";
      direction = "x,y,z : [" + Math.round(acceleration.x) + "," + Math.round(acceleration.y) + "," + Math.round(acceleration.z) + "]<BR/> Left Engine : " + Math.round(left) + ", Right Engine: " + Math.round(right);
      document.getElementById("vector").innerHTML = direction;
    }
  }
});

//Function for creating websocket connection
function connect() {
  window.WebSocket = window.WebSocket || window.MozWebSocket;
  var url = $("#cameraIP").val();
  socket = new WebSocket("ws://rcteer.swastibhat.com:8081");
  socket.addEventListener("open", function(event) {
    console.log("Connected");

    $('a.disabled').removeClass("disabled", false);
  });

  // Display messages received from the server
  socket.addEventListener("message", function(event) {
    console.log(typeof event.data);
    console.log(event.data);

    var urlObj = window.URL || window.webkitURL;
    //var imageUrl = urlCreator.createObjectURL(event.data);
    if (typeof event.data != "string") {
      var blob = event.data;
      var url = window.URL.createObjectURL(blob);
      console.log(blob);
      console.log('Created a png blob of size: ' + blob.size);
      console.log('Inserting an img...');
      document.querySelector("#image").src = url;
      document.querySelector("#image").onload = function() {
        window.URL.revokeObjectURL(this.src);
      }

      console.log('Blob URL is: ' + url);


    }

  });

  // Display any errors that occur
  socket.addEventListener("error", function(event) {
    console.log("Error: " + event);
  });

  socket.addEventListener("close", function(event) {
    open.disabled = false;
    console.log("Not Connected");
  });
};