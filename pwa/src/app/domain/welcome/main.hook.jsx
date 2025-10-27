import { useState } from "react";
import { useNavigate } from "react-router-dom";

export function useMain() {
  // LOCAL DECLARATION
  const navigate = useNavigate();
  const [buyData, setBuyData] = useState({
    paper: "",
    quantity: 0,
    change: 0,
  });
  // Add global isPaymentPage state
  const [isPaymentPage, setIsPaymentPage] = useState(false);

  return {
    buyData,
    setBuyData,
    navigate,
    isPaymentPage,
    setIsPaymentPage,
  };
}
