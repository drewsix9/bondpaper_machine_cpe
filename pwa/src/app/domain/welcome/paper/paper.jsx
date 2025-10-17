import { Button } from "primereact/button";
import { useMainContext } from "../main.context";

export default function Paper() {
  const { buyData, setBuyData, navigate } = useMainContext();

  return (
    <div className="flex justify-center items-center">
      <div className="flex space-x-10">
        <Button
          onClick={() => {
            setBuyData({ ...buyData, paper: "LONG" });
            navigate(`/system/quantity`);
          }}
          className="bg-blue-600 text-white font-bold px-24 py-20 text-4xl rounded-md"
        >
          Long
        </Button>
        <Button
          onClick={() => {
            setBuyData({ ...buyData, paper: "A4" });
            navigate(`/system/quantity`);
          }}
          className="bg-blue-600 text-white font-bold px-24 py-20 text-4xl rounded-md"
        >
          Short
        </Button>
      </div>
    </div>
  );
}
