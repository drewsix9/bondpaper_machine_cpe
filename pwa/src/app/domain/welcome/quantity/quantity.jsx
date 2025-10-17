import "primeicons/primeicons.css";
import { InputNumber } from "primereact/inputnumber";
import { useMainContext } from "../main.context";

export default function Quantity() {
  const { buyData, setBuyData, navigate } = useMainContext();
  return (
    <div>
      <InputNumber
        className="text-[5rem]"
        value={buyData?.quantity}
        onValueChange={(e) => setBuyData({ ...buyData, quantity: e.value })}
        showButtons
        buttonLayout="vertical"
        style={{ width: "8rem", height: "5rem" }}
        decrementButtonClassName="bg-blue-600 text-white py-5"
        incrementButtonClassName="bg-blue-600 text-white py-5"
        incrementButtonIcon="pi pi-plus"
        decrementButtonIcon="pi pi-minus"
        min={0}
      />
    </div>
  );
}
