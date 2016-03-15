defmodule OutletBoxEx do

  def start_hub() do
    {:ok, server} = Cure.load "./c_src/main" 
  end

  def start_loop do
    receive do
      {:cure_data, response} -> 
        IO.inspect response
    end
  end

end
