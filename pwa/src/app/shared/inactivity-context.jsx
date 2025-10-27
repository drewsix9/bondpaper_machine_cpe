import { createContext, useContext, useEffect, useState } from "react";
import { useNavigate } from "react-router-dom";
import { INACTIVITY_CONFIG } from "../shared/inactivity-config";

// Create the inactivity context
const InactivityContext = createContext();

// Custom hook to use the inactivity context
export function useInactivity() {
  return useContext(InactivityContext);
}

// Provider component that wraps the app and provides inactivity tracking
export function InactivityProvider({ children }) {
  const navigate = useNavigate();
  const [lastActivity, setLastActivity] = useState(Date.now());
  const [timer, setTimer] = useState(null);

  // Reset the inactivity timer when user activity is detected
  const resetInactivityTimer = () => {
    setLastActivity(Date.now());
  };

  // Set up the inactivity timer and event listeners
  useEffect(() => {
    // Clear any existing timer
    if (timer) {
      clearInterval(timer);
    }

    // Create a new timer that checks for inactivity
    const newTimer = setInterval(() => {
      const now = Date.now();
      const timeElapsed = now - lastActivity;

      if (timeElapsed >= INACTIVITY_CONFIG.timeout) {
        // Redirect to welcome page if inactive for too long
        navigate("/");
        // Reset the timer after redirect
        resetInactivityTimer();
      }
    }, 1000); // Check every second

    setTimer(newTimer);

    // Clean up the timer when the component unmounts
    return () => {
      clearInterval(newTimer);
    };
  }, [lastActivity, navigate]);

  // Set up event listeners for user activity
  useEffect(() => {
    // List of events to track for user activity
    const events = [
      "mousedown",
      "mousemove",
      "keypress",
      "scroll",
      "touchstart",
      "click",
    ];

    // Event handler function
    const handleUserActivity = () => {
      resetInactivityTimer();
    };

    // Add all event listeners
    events.forEach((event) => {
      window.addEventListener(event, handleUserActivity);
    });

    // Clean up event listeners
    return () => {
      events.forEach((event) => {
        window.removeEventListener(event, handleUserActivity);
      });
    };
  }, []);

  // Values to provide through the context
  const contextValue = {
    resetInactivityTimer,
    // Add any other values or functions you might need
  };

  return (
    <InactivityContext.Provider value={contextValue}>
      {children}
    </InactivityContext.Provider>
  );
}
