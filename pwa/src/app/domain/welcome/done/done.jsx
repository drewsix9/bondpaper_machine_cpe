import axios from "axios";
import { Button } from "primereact/button";
import { useEffect, useState } from "react";
import { BASE_URL } from "../../../shared/config";
import { useMainContext } from "../main.context";

export default function Done() {
  const { buyData, setBuyData, navigate } = useMainContext();
  const [done, setDone] = useState(false);
  const [dataSent, setDataSent] = useState(false);
  const [changeDispensed, setChangeDispensed] = useState(false);
  const [dispensingChange, setDispensingChange] = useState(false);
  const [dispensingError, setDispensingError] = useState(null);

  // Handle dispensing change when component mounts
  useEffect(() => {
    const dispenseChange = async () => {
      // Only dispense if there's change to give
      if (buyData.change && buyData.change > 0) {
        try {
          setDispensingChange(true);
          console.log(`Dispensing change: ${buyData.change} pesos`);

          // Call the change dispensing API endpoint
          const response = await axios.post(
            `${BASE_URL}/change/${buyData.change}`
          );

          if (response.status === 200) {
            console.log("Change dispensed successfully:", response.data);
            // Check if the response contains the "No response from device" error
            if (
              response.data &&
              response.data.error === "No response from device"
            ) {
              console.error("Device not responding:", response.data);
              setDispensingError(
                "Error: No response from coin dispenser. Please contact staff."
              );
            } else {
              setChangeDispensed(true);
            }
          } else {
            console.error("Error dispensing change:", response.data);
            setDispensingError(
              "Failed to dispense change. Please contact staff."
            );
          }
        } catch (error) {
          console.error("Change dispensing error:", error);
          setDispensingError("Error dispensing change. Please contact staff.");
        } finally {
          setDispensingChange(false);
        }
      } else {
        // No change to dispense, simply skip
        console.log("No change to dispense, skipping");
        setChangeDispensed(true);
      }
    };

    dispenseChange();
    // Including buyData.change in dependencies even though we only want this to run once
    // This avoids the ESLint warning
  }, [buyData.change]); // eslint-disable-line react-hooks/exhaustive-deps

  const handleDone = async () => {
    try {
      setDataSent(true);

      // First do a full device reset to clear any stuck state
      console.log("Resetting device state...");
      await axios.post(`${BASE_URL}/reset`);

      // Reset all states
      setDone(true);
    } catch (error) {
      console.error("Error resetting system:", error);
      // Even if there's an error, we still want to proceed to the next screen
      setDone(true);
    }
  };

  useEffect(() => {
    if (done) {
      // Reset all states and buyData after transaction is complete
      setBuyData({ paper: "", quantity: 0, change: 0 });
      setChangeDispensed(false);
      setDispensingChange(false);
      setDispensingError(null);
      navigate(`/system/paper`);
    }
  }, [done, setBuyData, navigate]);

  return (
    <div className="flex flex-col justify-center items-center h-screen space-y-8">
      {/* Status message for change dispensing */}
      {dispensingChange && (
        <div className="text-center">
          <h1 className="text-4xl font-bold mb-4">Dispensing Change...</h1>
          <p className="text-xl">
            Please wait while your change is being dispensed.
          </p>
          <p className="text-2xl font-semibold mt-4">
            Amount: ₱{buyData.change}
          </p>
        </div>
      )}

      {/* Error message if dispensing failed */}
      {dispensingError && (
        <div className="text-center">
          <h1 className="text-4xl font-bold mb-4 text-red-600">Error</h1>
          <p className="text-xl text-red-600">{dispensingError}</p>
        </div>
      )}

      {/* Success message and Done button */}
      {!dispensingChange && changeDispensed && !dataSent && (
        <div className="text-center">
          <h1 className="text-4xl font-bold mb-8 text-green-600">
            Transaction Complete!
          </h1>
          {buyData.change > 0 && (
            <p className="text-2xl mb-8">
              Your change of ₱{buyData.change} has been dispensed.
            </p>
          )}
          <Button
            onClick={handleDone}
            className="bg-blue-600 text-white font-bold px-8 py-5 text-4xl rounded-md"
          >
            Done
          </Button>
        </div>
      )}

      {/* Wait message after clicking done */}
      {dataSent && <h1 className="text-4xl font-bold">Please Wait...</h1>}
    </div>
  );
}
