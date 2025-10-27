import axios from "axios";
import { Button } from "primereact/button";
import { Dialog } from "primereact/dialog";
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
  const [showOutOfPaperDialog, setShowOutOfPaperDialog] = useState(false);
  const [refundMessage, setRefundMessage] = useState("");
  const location = useLocation();

  // Update global payment page state when location changes
  useEffect(() => {
    const currentlyOnPaymentPage = location.pathname === "/system/pay";
    setIsPaymentPage(currentlyOnPaymentPage);
    console.log(`Setting global isPaymentPage: ${currentlyOnPaymentPage}`);
  }, [location.pathname, setIsPaymentPage]);

  // Payment screen entry/exit handling with coinslot control
  useEffect(() => {
    if (!isPaymentPage) return; // Skip if not on payment page

    console.log("Payment screen entered - enabling coinslot");

    // Enable coinslot when entering payment page
    const enableCoinslot = async () => {
      try {
        const res = await axios.post(`${BASE_URL}/coinslot/on`);
        console.log("Coinslot enabled:", res.data);
      } catch (err) {
        console.error("Failed to enable coinslot:", err);
      }
    };

    enableCoinslot();

    return () => {
      console.log("Payment screen exited - disabling coinslot");

      // Disable coinslot when exiting payment page
      const disableCoinslot = async () => {
        try {
          const res = await axios.post(`${BASE_URL}/coinslot/off`);
          console.log("Coinslot disabled:", res.data);
        } catch (err) {
          console.error("Failed to disable coinslot:", err);
        }
      };

      disableCoinslot();
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

                // Disable coinslot immediately to prevent more coins being inserted
                console.log("Disabling coinslot...");
                try {
                  await axios.post(`${BASE_URL}/coinslot/off`);
                  console.log("Coinslot disabled successfully");
                } catch (coinslotErr) {
                  console.error("Failed to disable coinslot:", coinslotErr);
                }

                // Set loading state
                setDispensing(true);

                // Get paper type (SHORT or LONG) and quantity from buyData
                const paperType =
                  buyData?.paper === "A4" ? "SHORT" : buyData?.paper;
                const count = buyData?.quantity || 1;

                // **FINAL CHECK: Verify paper is still available before dispensing**
                console.log(`Checking if ${paperType} paper is available...`);
                const paperCheckRes = await axios.get(
                  `${BASE_URL}/paper/${paperType}`
                );

                if (!paperCheckRes.data?.has_paper) {
                  // Paper is out of stock!
                  console.error(`${paperType} paper is out of stock!`);

                  // Attempt full refund
                  if (amount > 0) {
                    console.log(`Attempting to refund ${amount} coins...`);
                    try {
                      const refundRes = await axios.post(
                        `${BASE_URL}/change/${amount}`
                      );
                      console.log("Refund response:", refundRes.data);
                      setRefundMessage(`Refunded ₱${amount}`);
                    } catch (refundErr) {
                      console.error("Refund failed:", refundErr);
                      setRefundMessage("Refund failed - Please contact staff");
                    }
                  } else {
                    setRefundMessage("No coins to refund");
                  }

                  // Show out of paper dialog
                  setDispensing(false);
                  setShowOutOfPaperDialog(true);
                  return;
                }

                // Paper is available, proceed with dispensing
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

      {/* Out of Paper Dialog */}
      <Dialog
        visible={showOutOfPaperDialog}
        onHide={() => {
          setShowOutOfPaperDialog(false);
          navigate("/");
        }}
        header="Out of Paper"
        modal
        style={{ width: "50vw" }}
        contentStyle={{
          padding: "2rem",
          backgroundColor: "#ffffff", // ✅ solid white background
        }}
        maskStyle={{
          backgroundColor: "rgba(0,0,0,0.6)", // ✅ darker backdrop
        }}
        className="rounded-xl shadow-lg" // optional: add border radius + shadow
      >
        <div className="flex flex-col items-center space-y-4">
          <i className="pi pi-exclamation-triangle text-red-600 text-6xl"></i>
          <h2 className="text-2xl font-bold text-center">
            Paper Not Available
          </h2>
          <p className="text-xl text-center">
            The selected paper type is currently out of stock.
          </p>
          {refundMessage && (
            <p className="text-xl text-center font-semibold text-green-600">
              {refundMessage}
            </p>
          )}
          <p className="text-xl text-center font-semibold">
            Please contact staff for assistance.
          </p>
          <Button
            label="Return to Home"
            onClick={() => {
              setShowOutOfPaperDialog(false);
              navigate("/");
            }}
            className="bg-blue-600 text-white px-6 py-3 text-xl rounded-md mt-4"
            autoFocus
          />
        </div>
      </Dialog>
    </div>
  );
}
