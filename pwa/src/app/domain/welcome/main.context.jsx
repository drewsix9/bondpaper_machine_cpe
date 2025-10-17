import { ReactNode, createContext, useContext } from "react";
import { useMain } from "./main.hook";

const MainContext = createContext({});
export function useMainContext() {
  return useContext(MainContext);
}

export function MainProvider({ children }) {
  return (
    <MainContext.Provider value={useMain()}>{children}</MainContext.Provider>
  );
}
