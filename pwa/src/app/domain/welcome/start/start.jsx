import axios from "axios";
import { Button } from "primereact/button";
import { ProgressSpinner } from "primereact/progressspinner";
import React, { useEffect, useState } from "react";
import { useLocation } from "react-router-dom";
import FormattedCurrency from "../../../shared/components/formatted-currency";
import { BASE_URL } from "../../../shared/config";
import { useMainContext } from "../main.context";

export default function Start() {
  const { buyData, setBuyData, navigate, isPaymentPage, setIsPaymentPage } =
    useMainContext();
  const [amount, setAmount] = useState(0);
  const [dispensing, setDispensing] = useState(false);
  const location = useLocation();

  // Update global payment page state when location changes
  useEffect(() => {
    const currentlyOnPaymentPage = location.pathname === "/system/pay";
    setIsPaymentPage(currentlyOnPaymentPage);
    console.log(`Setting global isPaymentPage: ${currentlyOnPaymentPage}`);
  }, [location.pathname, setIsPaymentPage]);

  // Payment screen entry/exit handling
  useEffect(() => {
    if (!isPaymentPage) return; // Skip if not on payment page

    console.log("Payment screen entered");

    return () => {
      console.log("Payment screen exited");
      // No need to do anything when exiting the page
    };
  }, [isPaymentPage]);

  // Create refs to store interval ID and mounted status outside of effects
  const intervalIdRef = React.useRef(null);
  const isMountedRef = React.useRef(true);

  // Function to stop polling
  const stopPolling = () => {
    if (intervalIdRef.current) {
      console.log(
        `Manually stopping coin polling (ID: ${intervalIdRef.current})`
      );
      clearInterval(intervalIdRef.current);
      intervalIdRef.current = null;
    }
  };

  useEffect(() => {
    // Update the ref when component mounts/unmounts
    isMountedRef.current = true;

    // Move handleGetCoinCount inside the effect to fix dependency issues
    const handleGetCoinCount = async () => {
      // Only execute when on payment page and component is still mounted
      if (!isPaymentPage || !isMountedRef.current) {
        return;
      }

      try {
        // Use the /coins endpoint as specified in main.py
        const res = await axios.get(`${BASE_URL}/coins`);
        if (res.status === 200 && isMountedRef.current) {
          // Extract coin count from the response - should be in format {"coins": number}
          const coinCount = res.data?.coins || 0;
          console.log("Current coin count:", coinCount);
          setAmount(coinCount);
        }
      } catch (err) {
        console.log("Error fetching coin count:", err);
      }
    };

    if (isPaymentPage) {
      console.log("Starting coin polling");
      // Poll every 500ms (twice per second) for more responsive UI updates
      intervalIdRef.current = setInterval(handleGetCoinCount, 1000);

      // Call once immediately to avoid initial delay
      handleGetCoinCount();
    }

    return () => {
      isMountedRef.current = false;
      stopPolling();
    };
  }, [isPaymentPage]); // Removed handleGetCoinCount from dependencies since it's inside the effect

  return (
    <div className="flex flex-1 space-y-5 flex-col items-center h-full">
      <div>
        <h1 className="text-2xl">Insert Coins</h1>
      </div>
      <div>
        <h1 className="text-2xl">
          Paper: {buyData?.paper === "A4" ? "Short" : buyData?.paper}
        </h1>
        <h1 className="text-2xl">Quantity: {buyData?.quantity}</h1>
      </div>
      <div>
        <h1 className="text-[8em] font-semibold">
          <FormattedCurrency amount={amount} />
        </h1>
      </div>
      <div>
        {dispensing ? (
          <div className="flex flex-col items-center justify-center p-6 space-y-4">
            <ProgressSpinner
              style={{ width: "80px", height: "80px" }}
              strokeWidth="4"
              fill="var(--surface-ground)"
              animationDuration=".8s"
            />
            <div className="text-2xl font-semibold text-center">
              Dispensing Papers please wait
            </div>
          </div>
        ) : (
          <Button
            disabled={buyData?.quantity > amount}
            onClick={async () => {
              try {
                // First, stop the coin polling interval
                console.log("Buy button clicked. Stopping coin polling...");
                stopPolling();

                // Set loading state
                setDispensing(true);

                // Get paper type (SHORT or LONG) and quantity from buyData
                const paperType =
                  buyData?.paper === "A4" ? "SHORT" : buyData?.paper;
                const count = buyData?.quantity || 1;

                // Calculate change amount (will be used in done.jsx)
                const changeAmount = amount - count;
                console.log(
                  `Calculating change: ${amount} - ${count} = ${changeAmount}`
                );

                // Update buyData with change amount
                setBuyData({
                  ...buyData,
                  change: changeAmount > 0 ? changeAmount : 0,
                });

                // Call the paper dispensing API endpoint
                console.log(
                  `Dispensing ${count} sheets of ${paperType} paper...`
                );
                const res = await axios.post(
                  `${BASE_URL}/paper/${paperType}/${count}`
                );

                if (res.status === 200) {
                  console.log("Paper dispensed successfully:", res.data);
                  navigate(`/system/done`);
                } else {
                  console.error("Error dispensing paper:", res.data);
                  setDispensing(false);
                  alert("Error dispensing paper. Please try again.");
                }
              } catch (err) {
                console.error("Failed to dispense paper:", err);
                setDispensing(false);
                alert("Failed to dispense paper. Please try again.");
              }
            }}
            className="bg-blue-600 text-white font-bold px-5 py-5 text-4xl rounded-md disabled:bg-gray-200"
          >
            Buy
          </Button>
        )}
      </div>
    </div>
  );
}
