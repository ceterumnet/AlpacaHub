 export const Focuser = function(props) {
     console.log("Focuser Component: ", props);
     return {
         device: props,
         setFocuserPosition(device) {
             console.log(`setting focuser pos to ${device.Position}`);
             console.log("Device: ", device);
             device.busy = true;
             fetch("/api/v1/" + device.DeviceType.toLowerCase() + "/" + device.DeviceNumber + "/move", {
                 method: 'PUT',
                 headers: {
                     'Content-Type': 'application/json'
                 },
                 body: `Position=${encodeURIComponent(device.Position)}`
             })
                 .then((_res) => _res.json())
                 .catch((e) => {
                     alert("problem setting focuser position!");
                     device.busy = false;
                 })
                 .then((_data) => {
                     if(_data.ErrorNumber === 0) {
                         // device.Position = null;
                         device.busy = true;
                         console.log("device should be busy...");
                         this.fetchDeviceDetails(device);
                     } else {
                         alert("Problem: " + _data.ErrorMessage);
                         this.fetchDeviceDetails(device);
                         device.busy = false;
                     }
                 });
         },
         haltFocuser(device) {
             device.busy = true;
             fetch("/api/v1/" + device.DeviceType.toLowerCase() + "/" + device.DeviceNumber + "/halt", {
                 method: 'PUT',
                 headers: {
                     'Content-Type': 'application/json'
                 }
             })
                 .then((_res) => _res.json())
                 .catch((e) => {
                     alert("problem halting focuser");
                     device.busy = false;
                 })
                 .then((_data) => {
                     if(_data.ErrorNumber === 0) {
                         // device.Position = null;
                         this.fetchDeviceDetails(device);
                         device.busy = true;
                     } else {
                         alert("Problem: " + _data.ErrorMessage);
                         this.fetchDeviceDetails(device);
                         device.busy = false;
                     }
                 });
         },
     };
 };
