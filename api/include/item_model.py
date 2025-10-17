from pydantic import BaseModel


# Define the request body model
class Item(BaseModel):
    paper: str
    quantity: int 
