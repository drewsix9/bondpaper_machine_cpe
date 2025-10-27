import { Button } from "primereact/button";
import { Dialog } from "primereact/dialog";
import { ProgressSpinner } from "primereact/progressspinner";
import { useState } from "react";
import { useMainContext } from "../main.context";
import { usePaperAvailability } from "../usePaperAvailability";

export default function Paper() {
  const { buyData, setBuyData, navigate } = useMainContext();
  const { paperStatus, loading } = usePaperAvailability();
  const [showOutOfStockDialog, setShowOutOfStockDialog] = useState(false);

  // Check if all paper types are out of stock
  const allOutOfStock =
    paperStatus.SHORT === false && paperStatus.LONG === false;

  // Show dialog when both are out of stock
  if (allOutOfStock && !showOutOfStockDialog) {
    setShowOutOfStockDialog(true);
  }

  const handlePaperSelection = (paperType) => {
    const apiPaperType = paperType === "A4" ? "SHORT" : paperType;

    // Check if this paper type is available
    if (paperStatus[apiPaperType] === false) {
      // Paper is out of stock, don't proceed
      return;
    }

    setBuyData({ ...buyData, paper: paperType });
    navigate(`/system/quantity`);
  };

  return (
    <>
      <div className="flex justify-center items-center">
        {loading ? (
          <ProgressSpinner
            style={{ width: "80px", height: "80px" }}
            strokeWidth="4"
          />
        ) : (
          <div className="flex space-x-10">
            {/* Long Paper Button */}
            <div className="relative">
              <Button
                onClick={() => handlePaperSelection("LONG")}
                disabled={paperStatus.LONG === false}
                className={`font-bold px-24 py-20 text-4xl rounded-md ${
                  paperStatus.LONG === false
                    ? "bg-gray-300 text-gray-500 cursor-not-allowed"
                    : "bg-blue-600 text-white"
                }`}
              >
                Long
              </Button>
              {paperStatus.LONG === false && (
                <div className="absolute inset-0 flex items-center justify-center bg-red-600 bg-opacity-90 rounded-md pointer-events-none">
                  <span className="text-white font-bold text-2xl">
                    OUT OF STOCK
                  </span>
                </div>
              )}
            </div>

            {/* Short Paper Button */}
            <div className="relative">
              <Button
                onClick={() => handlePaperSelection("A4")}
                disabled={paperStatus.SHORT === false}
                className={`font-bold px-24 py-20 text-4xl rounded-md ${
                  paperStatus.SHORT === false
                    ? "bg-gray-300 text-gray-500 cursor-not-allowed"
                    : "bg-blue-600 text-white"
                }`}
              >
                Short
              </Button>
              {paperStatus.SHORT === false && (
                <div className="absolute inset-0 flex items-center justify-center bg-red-600 bg-opacity-90 rounded-md pointer-events-none">
                  <span className="text-white font-bold text-2xl">
                    OUT OF STOCK
                  </span>
                </div>
              )}
            </div>
          </div>
        )}
      </div>

      {/* Out of Stock Dialog */}
      <Dialog
        visible={showOutOfStockDialog}
        onHide={() => {
          setShowOutOfStockDialog(false);
          navigate("/");
        }}
        header="Machine Unavailable"
        modal
        style={{ width: "50vw" }}
        contentStyle={{ padding: "2rem", backgroundColor: "#ffffff" }}
        maskStyle={{
          backgroundColor: "rgba(0,0,0,0.6)", // ✅ darker backdrop
        }}
      >
        <div className="flex flex-col items-center space-y-4">
          <i className="pi pi-exclamation-triangle text-red-600 text-6xl"></i>
          <h2 className="text-2xl font-bold text-center">Out of Paper</h2>
          <p className="text-xl text-center">
            All paper types are currently out of stock.
          </p>
          <p className="text-xl text-center font-semibold">
            Please contact staff for assistance.
          </p>
          <Button
            label="Go Back"
            onClick={() => {
              setShowOutOfStockDialog(false);
              navigate("/");
            }}
            className="bg-blue-600 text-white px-6 py-3 text-xl rounded-md mt-4"
          />
        </div>
      </Dialog>
    </>
  );
}
