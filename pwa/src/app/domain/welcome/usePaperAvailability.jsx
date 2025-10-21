import axios from "axios";
import { useEffect, useState } from "react";
import { BASE_URL } from "../../shared/config";

/**
 * Custom hook to check paper availability for both SHORT and LONG paper types
 * @returns {Object} Paper availability status and loading state
 */
export function usePaperAvailability() {
  const [paperStatus, setPaperStatus] = useState({
    SHORT: null, // null = loading, true = available, false = out of stock
    LONG: null,
  });
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);

  const checkPaperAvailability = async () => {
    setLoading(true);
    setError(null);

    try {
      // Check both paper types in parallel
      const [shortRes, longRes] = await Promise.all([
        axios.get(`${BASE_URL}/paper/SHORT`),
        axios.get(`${BASE_URL}/paper/LONG`),
      ]);

      setPaperStatus({
        SHORT: shortRes.data?.has_paper ?? false,
        LONG: longRes.data?.has_paper ?? false,
      });
    } catch (err) {
      console.error("Error checking paper availability:", err);
      setError(err.message);
      // On error, assume both are available (graceful degradation)
      setPaperStatus({
        SHORT: true,
        LONG: true,
      });
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    checkPaperAvailability();
  }, []);

  return {
    paperStatus,
    loading,
    error,
    recheckAvailability: checkPaperAvailability,
  };
}
